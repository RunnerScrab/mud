#include "server.h"
#include <sys/sysinfo.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/epoll.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#include "talloc.h"
#include "vector.h"
#include "client.h"
#include "threadpool.h"
#include "charvector.h"
#include "iohelper.h"

const char *g_ServerLogTypes[] = {"DEBUG", "STATUS", "ERROR"};
const int SERVERLOG_DEBUG = 0;
const int SERVERLOG_STATUS = 1;
const int SERVERLOG_ERROR = 2;

void* TestHandleClientInput(void* arg)
{
	printf("Received: %s\n", (char*) arg);
	return ((void*) 0);
}

void ServerLog(unsigned int code, const char* fmt, ...)
{
	va_list arglist;
	va_start(arglist, fmt);
	printf("%s: ", g_ServerLogTypes[code]);
	vprintf(fmt, arglist);
	printf("\n");
	va_end(arglist);
}


int CompClientSock(void* key, void* p)
{
	return (*((int*) key) - ((struct Client*) p)->ev_pkg.sockfd);
}

int Server_Configure(struct Server* server, const char* szAddr, unsigned short port)
{
	int opts = 1;
	server->sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if(FAILURE(server->sockfd))
	{
		ServerLog(SERVERLOG_ERROR, "Could not create socket.");
		return -1;
	}

	if (FAILURE(setsockopt(server->sockfd, SOL_SOCKET, SO_REUSEADDR, &opts, sizeof(int))))
	{
		ServerLog(SERVERLOG_ERROR, "Failed to set socket options.");
		return -1;
	}

	memset(&(server->addr_in), 0, sizeof(struct sockaddr_in));
	server->addr_in.sin_family = AF_INET;
	server->addr_in.sin_port = htons(port);
	inet_pton(AF_INET, szAddr, &(server->addr_in.sin_addr));

	// Configure epoll for the server
	server->evlist_len = 64;
	server->evlist = (struct epoll_event*) talloc(sizeof(struct epoll_event) * server->evlist_len);
	memset(server->evlist, 0, sizeof(struct epoll_event) * server->evlist_len);

	server->epfd = epoll_create(10);
	server->epkg.sockfd = server->sockfd;
	server->epkg.pData = 0;

	server->server_event.events = EPOLLIN | EPOLLEXCLUSIVE;
	server->server_event.data.ptr = &(server->epkg);

	server->cmdpipe_event.events = EPOLLIN | EPOLLEXCLUSIVE;
	server->cmdpipe_event.data.ptr = server->cmd_pipe;

	//Open a pipe for in-process server commands and have epoll watch the
	//read end
	if(FAILURE(pipe(server->cmd_pipe)) ||
		FAILURE(epoll_ctl(server->epfd, EPOLL_CTL_ADD,
					server->cmd_pipe[0], &(server->cmdpipe_event))))
	{
		ServerLog(SERVERLOG_ERROR, "FAILED TO CREATE COMMAND PIPE!");
		return -1;
	}


	if(FAILURE(epoll_ctl(server->epfd,
					EPOLL_CTL_ADD,
					server->sockfd,
					&(server->server_event))))
	{
		ServerLog(SERVERLOG_ERROR, "FATAL: Could not add server socket to epoll wait list!");
		Server_Teardown(server);
		return -1;
	}

	if(FAILURE(Vector_Create(&(server->clients), 64, Client_Destroy)))
	{
		ServerLog(SERVERLOG_ERROR, "FATAL: Failed to allocate memory for client list!");
		Server_Teardown(server);
		return -1;
	}

	MemoryPool_Init(&(server->mem_pool));
	return 0;
}

int Server_Teardown(struct Server* pServer)
{
	MemoryPool_Destroy(&(pServer->mem_pool));
	tfree(pServer->evlist);
	ThreadPool_Destroy(&(pServer->thread_pool));
	Vector_Destroy(&(pServer->clients));
	close(pServer->sockfd);
	close(pServer->cmd_pipe[0]);
	close(pServer->cmd_pipe[1]);
	return 0;
}

void Server_WriteToCmdPipe(struct Server* server, const char* msg, size_t msglen)
{
	write(server->cmd_pipe[1], msg, sizeof(char) * msglen);
}

int Server_Initialize(struct Server* server, unsigned int backlog)
{
	server->cpu_cores = get_nprocs();

	if(FAILURE(ThreadPool_Init(&(server->thread_pool), server->cpu_cores)))
	{
		ServerLog(SERVERLOG_ERROR, "FAILED TO INIT THREAD POOL!");
		return -1;
	}

	ServerLog(SERVERLOG_STATUS, "Server starting. %d cores detected.", server->cpu_cores);

	int result = bind(server->sockfd, (struct sockaddr*) &(server->addr_in),
			sizeof(struct sockaddr_in));

	if(FAILURE(result))
	{
		ServerLog(SERVERLOG_ERROR, "Could not bind to %s:%d.",
			inet_ntoa(server->addr_in.sin_addr), ntohs(server->addr_in.sin_port));
		return -1;
	}

	result = listen(server->sockfd, backlog);

	if(FAILURE(result))
	{
		ServerLog(SERVERLOG_ERROR, "Could not listen on %s:%d.",
			inet_ntoa(server->addr_in.sin_addr), ntohs(server->addr_in.sin_port));
		return -1;
	}

	ServerLog(SERVERLOG_STATUS, "Server listening on %s:%d.",
		inet_ntoa(server->addr_in.sin_addr), ntohs(server->addr_in.sin_port));

	return 0;
}

void Server_SendAllClients(struct Server* pServer, const char* fmt, ...)
{
	size_t idx = 0, z = Vector_Count(&(pServer->clients));
	struct Client *pClient = 0;
	va_list arglist;
	va_start(arglist, fmt);
	for(; idx < z; ++idx)
	{
		pClient = (struct Client*) Vector_At(&(pServer->clients), idx);
		Client_SendMsg(pClient, fmt, arglist);
	}
	va_end(arglist);
}

void Server_HandleClientDisconnect(struct Server* pServer,
				struct Client* pClient)
{
	ServerLog(SERVERLOG_STATUS, "Client disconnected.");
	size_t foundkey = 0;
	if(FAILURE(Vector_Find(&(pServer->clients), &(pClient->sock), CompClientSock, &foundkey)))
	{
		ServerLog(SERVERLOG_DEBUG, "DEBUG: Couldn't find client in vector!");
	}
	else
	{
		close(pClient->sock);
		//This should actually be done automatically, according to man epoll
		epoll_ctl(pServer->epfd, EPOLL_CTL_DEL, pClient->sock, 0);
		Vector_Remove(&(pServer->clients), foundkey);
	}

}

struct HandleUserInputTaskPkg
{
	struct Server* pServer;
	struct Client* pClient;
};

void* HandleUserInputTask(void* pArg)
{
	struct HandleUserInputTaskPkg* pPkg = pArg;
	struct Server* pServer = pPkg->pServer;
	struct Client* pClient = pPkg->pClient;

	cv_t clientinput;
	cv_init(&clientinput, 256);
	read_to_cv(pClient->sock, &clientinput, 256);
	size_t bytes_read = cv_len(&clientinput);
	cv_push(&clientinput, 0);
	if(bytes_read > 0)
	{
		/* DEMO CODE */
		printf("Received: %s\n", clientinput.data);

		if(strstr(clientinput.data, "kill"))
		{
			//This is obviously just for debugging!
			//TODO: Send a kill signal to the process

			Server_WriteToCmdPipe(pServer, "kill", 5);
		}
		/* END DEMO CODE */
	}
	else
	{
		Server_HandleClientDisconnect(pServer, pClient);
	}

	//Need to rearm the socket in the epoll interest list
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLONESHOT;
	ev.data.ptr = &(pClient->ev_pkg);
	epoll_ctl(pServer->epfd, EPOLL_CTL_MOD,
		pClient->sock, &ev);
	printf("client sock rearmed\n");
	cv_destroy(&clientinput);
	return 0;
}

void ReleaseTaskPkg(void* pArg)
{
	struct Server* pServer = ((struct HandleUserInputTaskPkg*) pArg)->pServer;
	MemoryPool_Free(&(pServer->mem_pool), sizeof(struct HandleUserInputTaskPkg), pArg);
}

void Server_HandleUserInput(struct Server* pServer, struct Client* pClient)
{
	printf("Dispatching task.\n");

	struct timespec curtime;
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &curtime);
	float interval = (curtime.tv_nsec - pClient->last_input_time.tv_nsec) / 1000000000.0;
	pClient->cmd_intervals[pClient->interval_idx % 3] = interval;
	++pClient->interval_idx;

	memcpy(&(pClient->last_input_time), &curtime, sizeof(struct timespec));

	unsigned char idx = 0;
	float sum = 0.f;
	for(; idx < 3; ++idx)
	{
		sum += pClient->cmd_intervals[idx];
	}
	float average_interval = sum / 3.f;
	Client_SendMsg(pClient, "You are sending commands at an average rate of %f per second.\n", average_interval);

	struct HandleUserInputTaskPkg* pPkg = MemoryPool_Alloc(&(pServer->mem_pool),
							sizeof(struct HandleUserInputTaskPkg));

//talloc(sizeof(struct HandleUserInputTaskPkg));
	pPkg->pServer = pServer;
	pPkg->pClient = pClient;

	if(FAILURE(ThreadPool_AddTask(&(pServer->thread_pool),
						HandleUserInputTask, 1, pPkg, ReleaseTaskPkg)))
	{
		ServerLog(SERVERLOG_ERROR,
			"Failed to add threadpool task!");
	}

}

int Server_AcceptClient(struct Server* server)
{
	unsigned int addrlen = 0;
	struct Client* pConnectingClient = Client_Create();


	int accepted_sock = accept(server->sockfd,
				&(pConnectingClient->addr), &addrlen);
	if(SUCCESS(accepted_sock))
	{
		ServerLog(SERVERLOG_STATUS, "Client connected.\n");
		pConnectingClient->sock = accepted_sock;
		pConnectingClient->ev_pkg.sockfd = accepted_sock;
		pConnectingClient->ev_pkg.pData = pConnectingClient;

		clock_gettime(CLOCK_PROCESS_CPUTIME_ID,	&(pConnectingClient->connection_time));
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID,	&(pConnectingClient->last_input_time));
		pConnectingClient->interval_idx = 0;
		memset(pConnectingClient->cmd_intervals, 0, sizeof(float) * 3);

		struct epoll_event clev;
		clev.events = EPOLLIN | EPOLLONESHOT;
		clev.data.ptr = &(pConnectingClient->ev_pkg);

		Vector_Push(&(server->clients), pConnectingClient);

#ifdef DEBUG
		Client_SendMsg(pConnectingClient, "*****The server is running as a DEBUG build*****\r\n");
#endif

		epoll_ctl(server->epfd, EPOLL_CTL_ADD, accepted_sock, &clev);
		return accepted_sock;
	}
	else
	{
		ServerLog(SERVERLOG_STATUS, "Client attempted and failed to connect.\n");
		return -1;
	}
}
