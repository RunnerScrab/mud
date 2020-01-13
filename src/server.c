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
#include "ansicolor.h"

const char *g_ServerLogTypes[] = {"DEBUG", "STATUS", "ERROR"};
const int SERVERLOG_DEBUG = 0;
const int SERVERLOG_STATUS = 1;
const int SERVERLOG_ERROR = 2;

#ifndef max
#define max(a, b) (a > b ? a : b)
#endif

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

void Server_SendClientMotd(struct Server* pServer, struct Client* pClient)
{
	Client_WriteTo(pClient, pServer->MOTD, strlen(pServer->MOTD));
}

int Server_LoadMOTD(struct Server* server)
{
	FILE* fp = fopen("motd.txt", "rb");
	if(!fp)
	{
		return -1;
	}
	fseek(fp, 0, SEEK_END);
	size_t len = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	server->MOTD = (char*) talloc(sizeof(char) * (len + 1));
	fread(server->MOTD, sizeof(char), len, fp);
	server->MOTD[len] = 0;
	fclose(fp);
	return 0;
}

void Server_FreeMOTD(struct Server* server)
{
	tfree(server->MOTD);
	server->MOTD = 0;
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
	server->addr_in.sin_addr.s_addr = INADDR_ANY;
	//inet_pton(AF_INET, szAddr, &(server->addr_in.sin_addr));

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

	pthread_mutex_init(&server->timed_queue_mtx, 0);
	prioq_create(&server->timed_queue, 32);

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

	pthread_mutex_init(&server->clients_mtx, 0);
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
	Server_FreeMOTD(pServer);
	asThreadCleanup();
	AngelScriptManager_ReleaseEngine(&pServer->as_manager);
	pthread_mutex_lock(&pServer->timed_queue_mtx);
	MemoryPool_Destroy(&(pServer->mem_pool));
	pthread_mutex_destroy(&pServer->timed_queue_mtx);
	prioq_destroy(&pServer->timed_queue);
	printf("Freeing server evlist.\n");
	tfree(pServer->evlist);
	ThreadPool_Destroy(&(pServer->thread_pool));
	pthread_mutex_lock(&pServer->clients_mtx);
	Vector_Destroy(&pServer->clients);
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

	ServerLog(SERVERLOG_STATUS, "Server starting. %d cores detected.", server->cpu_cores);

	int result = bind(server->sockfd, (struct sockaddr*) &server->addr_in,
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


	result = AngelScriptManager_InitEngine(&server->as_manager);
	if(FAILURE(result))
	{
		ServerLog(SERVERLOG_ERROR, "Failed to initialize AngelScript engine.\n");
		return -1;
	}

	result = AngelScriptManager_InitAPI(&server->as_manager, server);
	if(FAILURE(result))
	{
		ServerLog(SERVERLOG_ERROR, "Failed to init AS API.\n");
		return -1;
	}

	result = AngelScriptManager_LoadScripts(&server->as_manager, ".");
	if(FAILURE(result))
	{
		ServerLog(SERVERLOG_ERROR, "Failed to load game scripts.\n");
		return -1;
	}


	if(FAILURE(ThreadPool_Init(&server->thread_pool, &server->as_manager, max(server->cpu_cores - 2, 1))))
	{
		ServerLog(SERVERLOG_ERROR, "FAILED TO INIT THREAD POOL!");
		return -1;
	}

	Server_LoadMOTD(server);

	return 0;
}

void Server_SendAllClients(struct Server* pServer, const char* fmt, ...)
{
	pthread_mutex_lock(&pServer->clients_mtx);
	size_t idx = 0, z = Vector_Count(&pServer->clients);
	struct Client *pClient = 0;
	va_list arglist;
	va_start(arglist, fmt);
	for(; idx < z; ++idx)
	{
		pClient = (struct Client*) Vector_At(&pServer->clients, idx);
		Client_Sendf(pClient, fmt, arglist);
	}
	va_end(arglist);
	pthread_mutex_unlock(&pServer->clients_mtx);
}

void Server_HandleClientDisconnect(struct Server* pServer,
				struct Client* pClient)
{
	ServerLog(SERVERLOG_STATUS, "Client disconnected.");
	size_t foundkey = 0;

	pthread_mutex_lock(&pServer->clients_mtx);
	if(FAILURE(Vector_Find(&pServer->clients, &pClient->sock, CompClientSock, &foundkey)))
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
	pthread_mutex_unlock(&pServer->clients_mtx);

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
	//Rearm this socket in the epoll interest list.
	//We need to rearm the client socket after each time we respond to it.
	//EPOLLONESHOT is set to prevent an infinite task-dispatching loop, whose exact mechanism
	//I cannot presently remember; it was challenging to work out what was happening
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

void* TestTimedTask(void* pArgs)
{
	//TODO: Delete this
	struct Server* pServer = (struct Server*) pArgs;
	Server_SendAllClients(pServer, "HELLO TIMED TASK HERE\r\n\r\n");
	return (void*) 0;
}


void* HandleUserInputTask(void* pArg)
{
	//This is the user input processor task dispatched to a worker thread by the threadpool.
	//Handling of user commands should generally run from this point without dispatching another task
	//(except for commands that add a timed event and the like)
	struct HandleUserInputTaskPkg* pPkg = (struct HandleUserInputTaskPkg*) pArg;
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
			//I don't know why zdecompression would fail, or whether or not the
			//following code actually does anything useful in the event that it does
			ServerLog(SERVERLOG_ERROR, "ZDecompression FAILED on client input.");
			ZCompressor_Reset(&pClient->zstreams);
			RearmClientSocket(pServer, pClient);
			cv_destroy(&decompout);
			cv_destroy(&clientinput);
			return (void*) -1;
		}
		cv_swap(&decompout, &clientinput);
		cv_destroy(&decompout);
	}

	//Scan for IAC bytes
	if(memchr(clientinput.data, 255, clientinput.length))
	{
		cv_t normchars;
		cv_init(&normchars, 64);
		//Process IACs
		for(; idx < z; ++idx)
		{
			bHadIAC |= TelnetStream_ProcessByte(&pClient->tel_stream,
							clientinput.data[idx], &normchars);
		}

		if(bHadIAC)
		{
			//The user can in principle send telnet IACs and regular input
			//at the same time, which is why ProcessByte accumulates regular input
			//into normchars. After we finish responding to telnet commands, we
			//process the regular input as normal.
			cv_clear(&clientinput);
			cv_appendcv(&clientinput, &normchars);
		}
		cv_destroy(&normchars);
	}


	unsigned char inputcomplete = 0;
	cv_t* cbuf = &(pClient->input_buffer);
	cv_appendcv(cbuf, &clientinput);
	printf("Received %lu bytes.\n", bytes_read);
	//DebugPrintCV(&clientinput);
	if(cbuf->length >= 2)
	{
		//User commands end in a crlf if their client is in line mode,
		//and just a 13 otherwise. We have to account for both possibilities,
		//particularly since many clients are always in character mode before
		//and during negotiation

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
		//At this point, we have a complete user command and can process it
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
		else if(strstr(cbuf->data, "testtimer"))
		{
			//TODO: Delete this
			Server_AddTimedTask(pServer, TestTimedTask,
					time(0) + 5,
					(void*) pServer,
					0);
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
	//This function dispatches a task to handle the user's input
	struct timespec curtime;

	if(FAILURE(clock_gettime(CLOCK_BOOTTIME, &curtime)))
	{
		ServerLog(SERVERLOG_DEBUG, "CLOCK FAILED\n");
	}

	if(TimeDiffSecs(&curtime, &pClient->connection_time) >= 5.f)
	{
		//MUD clients are often in character mode upon first connecting
		//to perform negotiation, which means a very high command rate.
		//We don't want to throttle/boot a user because of or during negotiation.

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
				" and are being disconnected. Make sure your client is in line mode.\n", average_cps);
			Server_DisconnectClient(pServer, pClient);
			return;
		}
		else if(average_cps > 5.f)
		{
			Client_Sendf(pClient,
				"You are sending commands at an average rate of %f per second.\n", average_cps);
			RearmClientSocket(pServer, pClient);
			return;
		}
	}
	struct HandleUserInputTaskPkg* pPkg = (struct HandleUserInputTaskPkg*) MemoryPool_Alloc(&(pServer->mem_pool),
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
		clev.data.ptr = &(pConnectingClient->ev_pkg); //This is kind of convoluted, but it's EPOLL

		pthread_mutex_lock(&server->clients_mtx);
		Vector_Push(&server->clients, pConnectingClient);
		pthread_mutex_unlock(&server->clients_mtx);
		TelnetStream_SendPreamble(&pConnectingClient->tel_stream);

		Server_SendClientMotd(server, pConnectingClient);
#ifdef DEBUG
		Client_Sendf(pConnectingClient, "\r\n`#ff0000`*****The server is running as a DEBUG build*****`default`\r\n\r\n");
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
