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
#include "server.h"
#include "constants.h"
#include "as_manager.h"

#define SUCCESS(x) (x >= 0)
#define FAILURE(x) (x < 0)

struct TickThreadPkg
{
	struct Server* pServer;
	volatile char bIsRunning;
	size_t tick_delay;
};

struct CmdDispatchThreadPkg
{
	struct Server* pServer;
	volatile char bIsRunning;
	pthread_cond_t wakecond;
	pthread_mutex_t wakecondmtx;
};

void CmdDispatchThreadPkg_Init(struct CmdDispatchThreadPkg* pkg, struct Server* server)
{
	pkg->pServer = server;
	pkg->bIsRunning = 1;
	pthread_cond_init(&pkg->wakecond, 0);
	pthread_mutex_init(&pkg->wakecondmtx, 0);
}

void CmdDispatchThreadPkg_Destroy(struct CmdDispatchThreadPkg* pkg)
{
	pkg->bIsRunning = 0;
	pthread_mutex_lock(&pkg->wakecondmtx);
	pthread_cond_destroy(&pkg->wakecond);
	pthread_mutex_destroy(&pkg->wakecondmtx);
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

void* UserCommandDispatchThreadFn(void* pArgs)
{
	//The reason user command dispatch shouldn't be in the tick
	//thread is that command dispatch should not be limited to every server tick
	ServerLog(SERVERLOG_STATUS, "Running user command dispatch thread\n");
	struct CmdDispatchThreadPkg* pPkg = (struct CmdDispatchThreadPkg*) pArgs;
	struct Server* pServer = pPkg->pServer;
	time_t curtime;
	size_t idx = 0, len = 0;
	time_t min_delay = 0;
	while(pPkg->bIsRunning)
	{
		curtime = time(0);
		pthread_mutex_lock(&pServer->clients_mtx);

		for(idx = 0, len = Vector_Count(&pServer->clients);
		    idx < len; ++idx)
		{
			struct Client* pClient = (struct Client*) Vector_At(&pServer->clients, idx);
			pthread_mutex_lock(&pClient->cmd_queue_mtx);
			time_t delay = 0;
			while(prioq_get_size(&pClient->cmd_queue) > 0 &&
				curtime >= (delay = prioq_get_key_at(&pClient->cmd_queue, 0)))
			{
				struct ThreadTask* pTask = (struct ThreadTask*) prioq_extract_min(&pClient->cmd_queue);
				ThreadPool_AddTask(&pServer->thread_pool, pTask->taskfn, 0,
						pTask->pArgs, pTask->releasefn);
				MemoryPool_Free(&pClient->mem_pool, sizeof(struct ThreadTask), pTask);
			}

			pthread_mutex_unlock(&pClient->cmd_queue_mtx);
			if(delay > curtime)
			{
				min_delay = min_delay > delay ? delay : min_delay;
			}

		}

		pthread_mutex_unlock(&pServer->clients_mtx);

		if(min_delay > curtime)
		{
			sleep(curtime - min_delay);
		}
		else
		{
			pthread_cond_wait(&pPkg->wakecond, &pPkg->wakecondmtx);
			pthread_mutex_unlock(&pPkg->wakecondmtx);
		}

	}

	asThreadCleanup();
	return (void*) 0;
}

void* TickThreadFn(void* pArgs)
{
	//The idea here is that timed events are allocated from the
	//server's pool allocator and enqueued on their own priority
	//queue keyed by the time they are to execute.

	//In addition to conventional tick duties, this thread
	//checks the queue of timed events and sees which are ready
	//to execute. It dispatches those events which are ready to
	//the server's threadpool and does not run them itself.

	//The importance of this is that work to be done on a server
	//tick may occur simultaneously.

	struct TickThreadPkg* pPkg = (struct TickThreadPkg*) pArgs;
	struct Server* pServer = pPkg->pServer;
	const size_t tick_delay = pPkg->tick_delay;
	time_t curtime;

	while(pPkg->bIsRunning)
	{
		Server_SendAllClients(pServer, "Tick!\r\n\r\n");
		curtime = time(0);

		AngelScriptManager_RunWorldTick(&pServer->as_manager);

		//Dispatch ready server events
		pthread_mutex_lock(&pServer->timed_queue_mtx);
		while(prioq_get_size(&pServer->timed_queue) > 0 &&
			curtime >=
			prioq_get_key_at(&pServer->timed_queue, 0))
		{
			struct ThreadTask* pTask = (struct ThreadTask*) prioq_extract_min(&pServer->timed_queue);
			ThreadPool_AddTask(&pServer->thread_pool,
					pTask->taskfn, 0,
					pTask->pArgs, pTask->releasefn);
			MemoryPool_Free(&pServer->mem_pool, sizeof(struct ThreadTask), pTask);
		}
		pthread_mutex_unlock(&pServer->timed_queue_mtx);

		usleep(tick_delay);
	}
	ServerLog(SERVERLOG_STATUS, "Tickthread terminating.\n");
	asThreadCleanup();
	return (void*) 0;
}

int main(int argc, char** argv)
{
#ifdef DEBUG
	ServerLog(SERVERLOG_STATUS, "*****DEBUG BUILD*****");
#endif
	struct Server server;

	struct EvPkg* pEvPkg = 0;

	int ready = 0;
	int loop_ctr = 0;
	volatile char mudloop_running = 1;
	//TODO: Handle binding properly w/ ipv6 support
	if(FAILURE(Server_Configure(&server, "127.0.0.1", SERVER_PORT))
		|| FAILURE(Server_Initialize(&server, SERVER_LISTENQUEUELEN)))
	{
		//Server_Teardown(&server);
		return -1;
	}

	pthread_t tickthread;
	struct TickThreadPkg ttpkg;
	ttpkg.pServer = &server;
	ttpkg.bIsRunning = 1;
	ttpkg.tick_delay = 1000000;
	pthread_create(&tickthread, 0, TickThreadFn, (void*) &ttpkg);

	pthread_t cmddispatchthread;
	struct CmdDispatchThreadPkg cdtpkg;
	CmdDispatchThreadPkg_Init(&cdtpkg, &server);
	pthread_create(&cmddispatchthread, 0, UserCommandDispatchThreadFn, (void*) &cdtpkg);

	for(;mudloop_running;)
	{

		ready = epoll_wait(server.epfd, server.evlist, server.evlist_len, -1);

		if(ready == -1)
		{
			ServerLog(SERVERLOG_ERROR, "Ready -1\n");
			//break;
		}

		for(loop_ctr = 0; loop_ctr < ready; ++loop_ctr)
		{
			pEvPkg = (struct EvPkg*) (server.evlist[loop_ctr].data.ptr);
			if(pEvPkg->sockfd == server.sockfd)
			{
				Server_AcceptClient(&server);
			}
			//HACK: ugh, I should make "EvPkg" both more general and descriptive
			//Basically all it does is allow epoll to pass us back the socket
			//experiencing activity and a pointer to a client IF AND ONLY IF
			//it's actually a client.
			else if(server.evlist[loop_ctr].data.ptr == server.cmd_pipe)
			{
				//Received something on cmd pipe
				char buf[256] = {0};
				size_t bread = read(server.cmd_pipe[0], buf, 256);
				buf[bread - 1] = 0;
				printf("Received on cmd pipe: %s\n", buf);
				if(strstr(buf, "kill"))
				{
					Server_SendAllClients(&server, "\r\nServer going down!\r\n");
					mudloop_running = 0;
					break;
				}
			}
			else
			{
				Server_HandleUserInput(&server, (struct Client*) pEvPkg->pData);
			}

		}


	}


	ServerLog(SERVERLOG_STATUS, "Server shutting down.");

	ttpkg.bIsRunning = 0;
	pthread_join(tickthread, 0);
	cdtpkg.bIsRunning = 0;

	pthread_cond_broadcast(&cdtpkg.wakecond);
	pthread_join(cmddispatchthread, 0);
	CmdDispatchThreadPkg_Destroy(&cdtpkg);

	Server_Teardown(&server);
	printf("%d unfreed allocations.\n", toutstanding_allocs());
	return 0;
}
