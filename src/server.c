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
#include "rand.h"
#include "crypto.h"

#include "tickthread.h"

const char *g_ServerLogTypes[] = {"DEBUG", "STATUS", "ERROR"};
const int SERVERLOG_DEBUG = 0;
const int SERVERLOG_STATUS = 1;
const int SERVERLOG_ERROR = 2;

#ifndef max
#define max(a, b) (a > b ? a : b)
#endif
#ifndef min
#define min(a, b) (a < b ? a : b)
#endif


//Nonnegative if OK, -1 if player is clocked sending too much too fast
int Server_ClockPlayer(struct Server* pServer, struct Client* pClient, size_t data_sent);

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

void Server_AddTimedTask(struct Server* pServer, void* (*taskfn) (void*),
			time_t runtime, void* args,
			void (*argreleaserfn) (void*))
{
	struct ThreadTask* pTask = (struct ThreadTask*) MemoryPool_Alloc(&pServer->mem_pool, sizeof(struct ThreadTask));
	pTask->taskfn = taskfn;
	pTask->pArgs = args;
	pTask->releasefn = argreleaserfn;
	pthread_mutex_lock(&pServer->timed_queue_mtx);
	prioq_min_insert(&pServer->timed_queue, runtime, pTask);
	pthread_mutex_unlock(&pServer->timed_queue_mtx);
}

void Server_WriteToCmdPipe(struct Server* server, const char* msg, size_t msglen)
{
	write(server->cmd_pipe[1], msg, sizeof(char) * msglen);
}


void Server_SendAllClients(struct Server* pServer, const char* fmt, ...)
{
	pthread_rwlock_rdlock(&pServer->clients_rwlock);
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
	pthread_rwlock_unlock(&pServer->clients_rwlock);
}

void Server_HandleClientDisconnect(struct Server* pServer,
				struct Client* pClient)
{

	if(!pClient->bDisconnected)
	{
		ServerLog(SERVERLOG_STATUS, "Client disconnected.");
	}

	Client_Disconnect(pClient);
	size_t cref = Client_GetRefCount(pClient);
	ServerLog(SERVERLOG_DEBUG, "Client has a refcount of %lu\n", cref);
	if(!cref)
	{
		AngelScriptManager_CallOnPlayerDisconnect(&pServer->as_manager, pClient);
		size_t foundkey = 0;

		pthread_rwlock_wrlock(&pServer->clients_rwlock);
		if(FAILURE(Vector_Find(&pServer->clients, &pClient->sock, CompClientSock, &foundkey)))
		{
			ServerLog(SERVERLOG_DEBUG, "Couldn't find client in vector!");
		}
		else
		{
			ServerLog(SERVERLOG_DEBUG, "Removing client from client vector.");
			//This should actually be done automatically, according to man epoll
			epoll_ctl(pServer->epfd, EPOLL_CTL_DEL, pClient->sock, 0);
			printf("Client vec size: %lu\n", Vector_Count(&pServer->clients));
			Vector_Remove(&pServer->clients, foundkey);
			printf("Client vec size after remove: %lu\n", Vector_Count(&pServer->clients));
		}
		pthread_rwlock_unlock(&pServer->clients_rwlock);
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

	struct HandleUserInputTaskPkg* pkg = (struct HandleUserInputTaskPkg*) pArg;
	struct Server* pServer = pkg->pServer;
	struct Client* pClient = pkg->pClient;
	ServerLog(SERVERLOG_DEBUG, "Client ref is being decremented.");
	Client_ReleaseRef(pClient);

	if(pClient->bDisconnected && !Client_GetRefCount(pClient))
	{
		//If a client spams data and is disconnected, it's possible that
		//they will have multiple HandleUserInput tasks still enqueued after
		//their connection is terminated.
		Server_DisconnectClient(pServer, pkg->pClient);
	}

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
	ServerLog(SERVERLOG_DEBUG, "Entered HandleUserInputTask");
	if(pClient->bDisconnected)
	{
		ServerLog(SERVERLOG_DEBUG, "Client was already disconnected.");
		Server_DisconnectClient(pServer, pClient);
		return (void*) 0;
	}


	unsigned char bClientDisconnected = 0;
	cv_t clientinput;
	cv_init(&clientinput, CLIENT_MAXINPUTLEN);
	size_t bytes_read = read_to_cv(pClient->sock, &clientinput, 0, CLIENT_MAXINPUTLEN);


	if(Server_ClockPlayer(pServer, pClient, bytes_read) < 0)
	{
		ServerLog(SERVERLOG_ERROR, "Client sending data too fast!");
		Client_Sendf(pClient, "\r\n`@255;0;0`You are sending data too fast and are being disconnected.`default`\r\n");
		cv_destroy(&clientinput);
		Server_DisconnectClient(pServer, pClient);
		return (void*) -1;
	}

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
		cv_init(&normchars, 128);
		//Process IACs
		for(; idx < z; ++idx)
		{
			register unsigned char ch = clientinput.data[idx];
			if(ch)
			{
				bHadIAC |= TelnetStream_ProcessByte(&pClient->tel_stream, ch, &normchars);
			}
			else
			{
				//We received a 0 byte, which, outside of the TRANSMIT BINARY command (which we will not support),
				//is possibly shellcode.
				pClient->tel_stream.state = TELSTATE_ERROR;
				break;
			}
		}

		if(TELSTATE_ERROR == pClient->tel_stream.state)
		{

			Client_Sendf(pClient, "`red`Your client appears to be sending erroneous telnet commands.`default`\r\n");
			cv_destroy(&clientinput);
			Server_DisconnectClient(pServer, pClient);
			return (void*) -1;
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
	pthread_mutex_unlock(&pClient->connection_state_mtx);

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
			//in CHAR mode
			--cbuf->length;
			cv_push(cbuf, 0);
			inputcomplete = 1;
		}
	}

	if(bytes_read > 0 && inputcomplete)
	{
		//At this point, we have a complete user command and can process it
		/* DEMO CODE */
		printf("Attemping to process: %s\n", cbuf->data);
		char out[64] = {0};
		if(!inet_ntop(pClient->addr.sin_family, (void*) &pClient->addr.sin_addr, out, sizeof(char) * 64))
		{
			ServerLog(SERVERLOG_ERROR, "Couldn't convert client address.");
		}
		/* Client_Sendf(pClient, "You (%s) sent: %s\r\n\r\n",
		   out, cbuf->data);*/

		// TODO: Process data here
		ServerLog(SERVERLOG_DEBUG, "Calling AS CallOnPlayerInput.");
		AngelScriptManager_CallOnPlayerInput(&pServer->as_manager, pClient, cbuf->data);
		ServerLog(SERVERLOG_DEBUG, "Returned from calling AS CallOnPlayerInput.");

		if(!strncmp(cbuf->data, "kill", min(4, bytes_read)))
		{
			//This is obviously just for debugging!
			//TODO: Get rid of this or put it in an admin only command

			Server_WriteToCmdPipe(pServer, "kill", 5);
		}
		else if(!strncmp(cbuf->data, "quit", min(4, bytes_read)))
		{
			Server_DisconnectClient(pServer, pClient);
			bClientDisconnected = 1;
		}
		else if(!strncmp(cbuf->data, "testtimer", min(9, bytes_read)))
		{
			//TODO: Delete this
			Server_AddTimedTask(pServer, TestTimedTask,
					time(0) + 5,
					(void*) pServer,
					0);
		}
		else if(!strncmp(cbuf->data, "tc", min(2, bytes_read)))
		{
			ServerLog(SERVERLOG_STATUS, "Queueing user command.");
			struct timespec current_ts;
			clock_gettime(CLOCK_MONOTONIC, &current_ts);
			current_ts.tv_sec += 6;
			Client_QueueCommand(pClient, TestTimedTask, current_ts.tv_sec,
					current_ts.tv_nsec, (void*) pServer, 0);
		}

		if(!bClientDisconnected)
		{
			cv_clear(cbuf);
		}
		/* END DEMO CODE */
	}
	else if(!bytes_read)
	{
		Server_HandleClientDisconnect(pServer, pClient);
		bClientDisconnected = 1;
	}

	if(!bClientDisconnected)
	{
		//Need to rearm the socket in the epoll interest list
		RearmClientSocket(pServer, pClient);
	}
	cv_destroy(&clientinput);
	return 0;
}

float TimeDiffSecs(struct timespec* b, struct timespec* a)
{
	return (b->tv_sec - a->tv_sec) +
		(b->tv_nsec - a->tv_nsec)/1000000000.0;
}

int Server_ClockPlayer(struct Server* pServer, struct Client* pClient, size_t data_sent)
{
	struct timespec curtime;

	if(FAILURE(clock_gettime(CLOCK_BOOTTIME, &curtime)))
	{
		ServerLog(SERVERLOG_DEBUG, "CLOCK FAILED\n");
	}

	//MUD clients are often in character mode upon first connecting
	//to perform negotiation, which means a very high command rate.
	//We don't want to throttle/boot a user because of or during negotiation.

	//TODO: Change this to a bitrate limit
	float interval = TimeDiffSecs(&curtime, &pClient->last_input_time);

	pClient->bytes_sent_at_intervals[pClient->interval_idx] = data_sent;
	pClient->cmd_intervals[pClient->interval_idx] = interval;

	pClient->interval_idx = (pClient->interval_idx + 1) % CLIENT_STOREDCMDINTERVALS;

	memcpy(&(pClient->last_input_time), &curtime, sizeof(struct timespec));

	unsigned char idx = 0;
	float sum = 0.f, bsum = 0.f;
	for(; idx < CLIENT_STOREDCMDINTERVALS; ++idx)
	{
		bsum += pClient->bytes_sent_at_intervals[idx];
		sum += pClient->cmd_intervals[idx];
	}

	float average_bps = bsum/sum;

	if(bsum >= CLIENT_MAXINPUTLEN &&  average_bps >= CLIENT_MAXBPS)
	{
		ServerLog(SERVERLOG_STATUS, "Client is being disconnected for exceeding maximum transmission rate. (%f bytes/s)", average_bps);
		return -1;
	}
	else if(bsum >= CLIENT_MAXINPUTLEN && average_bps > (CLIENT_MAXBPS / 2))
	{
		Client_Sendf(pClient,
			"`#255;0;0`You are sending data at an average rate of %f bytes/second. You will be kicked if this goes over %d.`default`\r\n", average_bps,
			CLIENT_MAXBPS);
		RearmClientSocket(pServer, pClient);
		return 0;
	}


	return 0;

}

void Server_HandleUserInput(struct Server* pServer, struct Client* pClient)
{
	//This function dispatches a task to handle the user's input
	struct HandleUserInputTaskPkg* pPkg = (struct HandleUserInputTaskPkg*) MemoryPool_Alloc(&(pServer->mem_pool),
												sizeof(struct HandleUserInputTaskPkg));

	pPkg->pServer = pServer;
	pPkg->pClient = pClient;
	Client_AddRef(pClient);
	ServerLog(SERVERLOG_DEBUG, "Client ref is being incremented.");
	if(FAILURE(ThreadPool_AddTask(&(pServer->thread_pool),
						HandleUserInputTask, 1, pPkg,
						ReleaseHandleUserInputTaskPkg)))
	{
		Client_ReleaseRef(pClient);
		ServerLog(SERVERLOG_ERROR,
			"Failed to add threadpool task!");
		RearmClientSocket(pServer, pClient);
	}
}

int Server_AcceptClient(struct Server* server)
{
	unsigned int addrlen = sizeof(struct sockaddr);
	struct Client* pConnectingClient = 0;

	struct sockaddr connaddr;

	int accepted_sock = accept(server->sockfd,
				&connaddr, &addrlen);
	if(SUCCESS(accepted_sock))
	{
		ServerLog(SERVERLOG_STATUS, "Client connected.\n");

		pConnectingClient = Client_Create(accepted_sock, server, &server->cmd_dispatch_thread);
		memcpy(&pConnectingClient->addr, &connaddr, sizeof(struct sockaddr_in));
		struct epoll_event clev;
		clev.events = EPOLLIN | EPOLLONESHOT;
		clev.data.ptr = &(pConnectingClient->ev_pkg); //This is kind of convoluted, but it's EPOLL


		pthread_rwlock_wrlock(&server->clients_rwlock);
		Vector_Push(&server->clients, pConnectingClient);
		pthread_rwlock_unlock(&server->clients_rwlock);

		TelnetStream_SendPreamble(&pConnectingClient->tel_stream);

		Server_SendClientMotd(server, pConnectingClient);
		AngelScriptManager_CallOnPlayerConnect(&server->as_manager, pConnectingClient);
#ifdef DEBUG
		Client_Sendf(pConnectingClient,
			"\r\n`#ff0000`*****The server is running as a DEBUG build*****`default`\r\n\r\n");
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
