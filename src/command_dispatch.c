#include "command_dispatch.h"
#include "server.h"
#include <sys/time.h>

void CmdDispatchThread_Init(struct CmdDispatchThread* dispatchthread, struct Server* server)
{
	dispatchthread->pServer = server;
	dispatchthread->bIsRunning = 1;
	pthread_cond_init(&dispatchthread->wakecond, 0);
	pthread_mutex_init(&dispatchthread->wakecondmtx, 0);

	pthread_create(&dispatchthread->thread, 0, UserCommandDispatchThreadFn,
		(void*) dispatchthread);
}

void CmdDispatchThread_Stop(struct CmdDispatchThread* dispatchthread)
{
	ServerLog(SERVERLOG_STATUS, "Stopping cmd dispatch thread.");
	dispatchthread->bIsRunning = 0;
	pthread_cond_broadcast(&dispatchthread->wakecond);
	ServerLog(SERVERLOG_STATUS, "Wake broadcast");
	pthread_join(dispatchthread->thread, 0);
	ServerLog(SERVERLOG_STATUS, "Cmd dispatch thread joined.");
}

void CmdDispatchThread_Destroy(struct CmdDispatchThread* dispatchthread)
{
	pthread_cond_destroy(&dispatchthread->wakecond);
	pthread_mutex_destroy(&dispatchthread->wakecondmtx);
}


void* UserCommandDispatchThreadFn(void* pArgs)
{
	//The reason user command dispatch shouldn't be in the tick
	//thread is that command dispatch should not be limited to every server tick
	ServerLog(SERVERLOG_STATUS, "Running user command dispatch thread");
	struct CmdDispatchThread* pThreadData = (struct CmdDispatchThread*) pArgs;
	struct Server* pServer = pThreadData->pServer;
	time_t curtime;
	size_t idx = 0, len = 0;
	time_t min_delay = 0;
	struct timespec ts;
	while(pThreadData->bIsRunning)
	{
		printf("Top of loop\n");
		curtime = time(0);
		pthread_mutex_lock(&pServer->clients_mtx);
		min_delay = 0;
		for(idx = 0, len = Vector_Count(&pServer->clients);
		    idx < len; ++idx)
		{
			struct Client* pClient = (struct Client*) Vector_At(&pServer->clients, idx);
			pthread_mutex_lock(&pClient->cmd_queue_mtx);
			time_t delay = 0;
			while(prioq_get_size(&pClient->cmd_queue) > 0 &&
				curtime >= (delay = prioq_get_key_at(&pClient->cmd_queue, 0)))
			{
				ServerLog(SERVERLOG_DEBUG, "Found a user command which should have fired %lds from now.",
					delay - curtime);


				struct ThreadTask* pTask = (struct ThreadTask*) prioq_extract_min(&pClient->cmd_queue);
				ThreadPool_AddTask(&pServer->thread_pool, pTask->taskfn, 0,
						pTask->pArgs, pTask->releasefn);
				MemoryPool_Free(&pClient->mem_pool, sizeof(struct ThreadTask), pTask);
			}

			pthread_mutex_unlock(&pClient->cmd_queue_mtx);

			min_delay = (!min_delay || min_delay > delay) ? delay : min_delay;

		}

		pthread_mutex_unlock(&pServer->clients_mtx);



		ts.tv_sec = time(0) + 1;
		ts.tv_nsec = 0;
		printf("Waiting\n");
		pthread_cond_timedwait(&pThreadData->wakecond, &pThreadData->wakecondmtx, &ts);
		printf("Wait done\n");
		/*
		if((min_delay - curtime) > 0)
		{
			printf("Sleeping for %ld seconds\n", min_delay - curtime);
			//sleep(min_delay - curtime);
			struct timespec ts;
			ts.tv_sec = min_delay;
			ts.tv_nsec = 0;
			pthread_cond_timedwait(&pThreadData->wakecond, &pThreadData->wakecondmtx, &ts);
		}
		else
		{
			printf("Waiting on cond var.\n");
			pthread_cond_wait(&pThreadData->wakecond, &pThreadData->wakecondmtx);
		}
		*/

	}
	printf("Leaving loop\n");
	asThreadCleanup();
	return (void*) 0;
}
