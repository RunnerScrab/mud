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
#include <signal.h>

#include "talloc.h"
#include "vector.h"
#include "client.h"
#include "threadpool.h"
#include "server.h"
#include "constants.h"
#include "as_manager.h"


#define SUCCESS(x) (x >= 0)
#define FAILURE(x) (x < 0)

static struct Server* g_pServer = 0;

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

void HandleKillSig(int sig)
{
	ServerLog(SERVERLOG_STATUS, "Received SIGINT from terminal!");
	Server_WriteToCmdPipe(g_pServer, "kill", 5);
}

int main(int argc, char** argv)
{
#ifdef DEBUG
	ServerLog(SERVERLOG_STATUS, "*****DEBUG BUILD*****");
#endif
	struct Server server;
	g_pServer = &server;

	struct EvPkg* pEvPkg = 0;

	int ready = 0;
	int loop_ctr = 0;
	volatile char mudloop_running = 1;

	//TODO: Handle binding properly w/ ipv6 support
	if(FAILURE(Server_Start(&server)))
	{
		return -1;
	}

	signal(SIGINT, HandleKillSig);

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
	Server_Teardown(&server);
	printf("%d unfreed allocations.\n", toutstanding_allocs());
	return 0;
}
