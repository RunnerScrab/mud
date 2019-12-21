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
#include "telnet.h"
#include "zcompressor.h"

const char *g_ServerLogTypes[] = {"DEBUG", "STATUS", "ERROR"};
const int SERVERLOG_DEBUG = 0;
const int SERVERLOG_STATUS = 1;
const int SERVERLOG_ERROR = 2;

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
		Client_Sendf(pClient, fmt, arglist);
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

void Server_DisconnectClient(struct Server* pServer, struct Client* pClient)
{
	Server_HandleClientDisconnect(pServer, pClient);
}

struct HandleUserInputTaskPkg
{
	struct Server* pServer;
	struct Client* pClient;
};

void ReleaseHandleUserInputTaskPkg(void* pArg)
{
	struct Server* pServer = ((struct HandleUserInputTaskPkg*) pArg)->pServer;
	MemoryPool_Free(&(pServer->mem_pool), sizeof(struct HandleUserInputTaskPkg), pArg);
}

static void RearmClientSocket(struct Server* pServer, struct Client* pClient)
{
	//Rearm this socket in the epoll interest list
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLONESHOT;
	ev.data.ptr = &(pClient->ev_pkg);
	epoll_ctl(pServer->epfd, EPOLL_CTL_MOD,
		pClient->sock, &ev);
}

void DebugPrintCV(cv_t* buf)
{
	size_t idx = 0, z = buf->capacity;
	for(; idx < z; ++idx)
	{
		printf(idx < z - 1 ? "%d," : "%d\n", 255 & cv_at(buf, idx));
	}
}

void* HandleUserInputTask(void* pArg)
{
	struct HandleUserInputTaskPkg* pPkg = pArg;
	struct Server* pServer = pPkg->pServer;
	struct Client* pClient = pPkg->pClient;

	cv_t clientinput;
	cv_init(&clientinput, CLIENT_MAXINPUTLEN);
	size_t bytes_read = read_to_cv(pClient->sock, &clientinput, 0, CLIENT_MAXINPUTLEN);

	size_t idx = 0, z = bytes_read;
	char bHadIAC = 0;

	if(pClient->tel_stream.opts.b_mccp3)
	{
		cv_t decompout;
		cv_init(&decompout, CLIENT_MAXINPUTLEN);
		if(FAILURE(ZCompressor_DecompressData(&pClient->zstreams, &clientinput, &decompout)))
		{
			printf("Decompression FAILED\n");
		}
		else
		{
			printf("Decompressed %lu bytes to %lu\n", clientinput.length, decompout.length);
		}
		cv_swap(&decompout, &clientinput);
		cv_destroy(&decompout);
	}

	if(memchr(clientinput.data, 255, clientinput.length))
	{
		for(; idx < z; ++idx)
		{
			bHadIAC |= TelnetStream_ProcessByte(&pClient->tel_stream,
							clientinput.data[idx]);
		}
	}

	if(bHadIAC)
	{
		cv_clear(&clientinput);
	}

	unsigned char inputcomplete = 0;
	cv_t* cbuf = &(pClient->input_buffer);
	cv_appendcv(cbuf, &clientinput);
	printf("Received %lu bytes.\n", bytes_read);
	//DebugPrintCV(&clientinput);
	if(cbuf->length >= 2)
	{
		//Look for a crlf
		if(((cbuf->data[cbuf->length - 2] << 8 )
				| cbuf->data[cbuf->length - 1])
			== ((13<<8)|10))
		{
			cbuf->length -= 2;
			cv_push(cbuf, 0);
			inputcomplete = 1;
		}
		else if(cbuf->data[cbuf->length - 2] == 13)
		{
			//in CHAR mode - do we really want to support this
			--cbuf->length;
			cv_push(cbuf, 0);
			inputcomplete = 1;
		}
	}

	if(bytes_read > 0 && inputcomplete)
	{
		/* DEMO CODE */
		printf("Received: %s\n", cbuf->data);
		Client_Sendf(pClient, "You sent: %s\r\n\r\n",
			cbuf->data);
		// TODO: Process data here


		if(strstr(cbuf->data, "kill"))
		{
			//This is obviously just for debugging!
			//TODO: Get rid of this or put it in an admin only command

			Server_WriteToCmdPipe(pServer, "kill", 5);
		}
		cv_clear(cbuf);
		/* END DEMO CODE */
	}
	else if(!bytes_read)
	{
		Server_HandleClientDisconnect(pServer, pClient);
	}

	//Need to rearm the socket in the epoll interest list
	RearmClientSocket(pServer, pClient);
	cv_destroy(&clientinput);
	return 0;
}

float TimeDiffSecs(struct timespec* b, struct timespec* a)
{
	return (b->tv_sec - a->tv_sec) +
		(b->tv_nsec - a->tv_nsec)/1000000000.0;
}

void Server_HandleUserInput(struct Server* pServer, struct Client* pClient)
{
	struct timespec curtime;

	if(FAILURE(clock_gettime(CLOCK_BOOTTIME, &curtime)))
	{
		ServerLog(SERVERLOG_DEBUG, "CLOCK FAILED\n");
	}

	if(TimeDiffSecs(&curtime, &pClient->connection_time) >= 5.f)
	{
		float interval = TimeDiffSecs(&curtime, &pClient->last_input_time);

		pClient->cmd_intervals[pClient->interval_idx] = interval;
		pClient->interval_idx = (pClient->interval_idx + 1) % CLIENT_STOREDCMDINTERVALS;
		memcpy(&(pClient->last_input_time), &curtime, sizeof(struct timespec));

		unsigned char idx = 0;
		float sum = 0.f;
		for(; idx < CLIENT_STOREDCMDINTERVALS; ++idx)
		{
			sum += pClient->cmd_intervals[idx];
		}
		float average_cps = ((float) CLIENT_STOREDCMDINTERVALS) / sum;

		if(average_cps > CLIENT_MAXCMDRATE)
		{
			ServerLog(SERVERLOG_STATUS, "Client is being disconnected for exceeding command rate. (%f cmds/s)", average_cps);
			Client_Sendf(pClient,
				"You are sending commands at an average rate of %f per second"
				" and are being disconnected.\n", average_cps);
			Server_DisconnectClient(pServer, pClient);
			return;
		}
		else if(average_cps > 5.f)
		{
			Client_Sendf(pClient, "You are sending commands at an average rate of %f per second.\n", average_cps);
			RearmClientSocket(pServer, pClient);
			return;
		}
	}
	struct HandleUserInputTaskPkg* pPkg = MemoryPool_Alloc(&(pServer->mem_pool),
							sizeof(struct HandleUserInputTaskPkg));

	pPkg->pServer = pServer;
	pPkg->pClient = pClient;

	if(FAILURE(ThreadPool_AddTask(&(pServer->thread_pool),
						HandleUserInputTask, 1, pPkg,
						ReleaseHandleUserInputTaskPkg)))
	{
		ServerLog(SERVERLOG_ERROR,
			"Failed to add threadpool task!");
		RearmClientSocket(pServer, pClient);
	}

}

int Server_AcceptClient(struct Server* server)
{
	unsigned int addrlen = 0;
	struct Client* pConnectingClient = 0;


	int accepted_sock = accept(server->sockfd,
				&(pConnectingClient->addr), &addrlen);
	if(SUCCESS(accepted_sock))
	{
		ServerLog(SERVERLOG_STATUS, "Client connected.\n");
		pConnectingClient = Client_Create(accepted_sock);
		struct epoll_event clev;
		clev.events = EPOLLIN | EPOLLONESHOT;
		clev.data.ptr = &(pConnectingClient->ev_pkg);

		Vector_Push(&(server->clients), pConnectingClient);
		TelnetStream_SendPreamble(&pConnectingClient->tel_stream);
#ifdef DEBUG
		Client_Sendf(pConnectingClient, "*****The server is running as a DEBUG build*****\r\n");
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
