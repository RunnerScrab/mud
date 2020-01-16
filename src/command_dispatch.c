#include "command_dispatch.h"
#include "server.h"
#include <sys/time.h>

int CmdDispatchThread_Init(struct CmdDispatchThread* dispatchthread, struct Server* server)
{
	dispatchthread->pServer = server;
	dispatchthread->bIsRunning = 1;
	pthread_condattr_init(&dispatchthread->wakecondattr);
	if(pthread_condattr_setclock(&dispatchthread->wakecondattr, CLOCK_MONOTONIC) < 0)
	{
		ServerLog(SERVERLOG_ERROR, "Failed to set cond clock attribute!");
		pthread_condattr_destroy(&dispatchthread->wakecondattr);
		return -1;
	}
	pthread_cond_init(&dispatchthread->wakecond, &dispatchthread->wakecondattr);
	pthread_mutex_init(&dispatchthread->wakecondmtx, 0);

	pthread_create(&dispatchthread->thread, 0, UserCommandDispatchThreadFn,
		(void*) dispatchthread);
	return 0;
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
	pthread_condattr_destroy(&dispatchthread->wakecondattr);
	pthread_mutex_destroy(&dispatchthread->wakecondmtx);
}

inline void ZeroTs(struct timespec* ts)
{
	ts->tv_sec = 0;
	ts->tv_nsec = 0;
}

inline int IsTsNonZero(struct timespec* ts)
{
	return ts->tv_sec && ts->tv_nsec;
}

void* UserCommandDispatchThreadFn(void* pArgs)
{
	//The reason user command dispatch shouldn't be in the tick
	//thread is that command dispatch should not be limited to every server tick
	ServerLog(SERVERLOG_STATUS, "Running user command dispatch thread");
	struct CmdDispatchThread* pThreadData = (struct CmdDispatchThread*) pArgs;
	struct Server* pServer = pThreadData->pServer;
	size_t idx = 0, len = 0;

	struct timespec current_ts, delay_ts, min_delay_ts;

	while(pThreadData->bIsRunning)
	{
		if(clock_gettime(CLOCK_MONOTONIC, &current_ts) < 0)
		{
			ServerLog(SERVERLOG_ERROR, "Failed to get current timestamp from system clock.");
			continue;
		}

		pthread_mutex_lock(&pServer->clients_mtx);

		ZeroTs(&min_delay_ts);

		for(idx = 0, len = Vector_Count(&pServer->clients);
		    idx < len; ++idx)
		{
			struct Client* pClient = (struct Client*) Vector_At(&pServer->clients, idx);
			pthread_mutex_lock(&pClient->cmd_queue_mtx);
			ZeroTs(&delay_ts);
			while(hrt_prioq_get_size(&pClient->cmd_queue) > 0)
			{
				hrt_prioq_get_key_at(&pClient->cmd_queue, 0, &delay_ts);
				if(CmpTs(&current_ts, &delay_ts) >= 0)
				{
					struct ThreadTask* pTask = (struct ThreadTask*) hrt_prioq_extract_min(&pClient->cmd_queue);
					ThreadPool_AddTask(&pServer->thread_pool, pTask->taskfn, 0,
							pTask->pArgs, pTask->releasefn);
					MemoryPool_Free(&pClient->mem_pool, sizeof(struct ThreadTask), pTask);

				}
				else
				{
					if(!IsTsNonZero(&min_delay_ts) ||
						(IsTsNonZero(&delay_ts) && CmpTs(&min_delay_ts, &delay_ts) == 1))
					{
						min_delay_ts.tv_sec = delay_ts.tv_sec + 1;
						min_delay_ts.tv_nsec = delay_ts.tv_nsec;
					}
					break;
				}
			}

			pthread_mutex_unlock(&pClient->cmd_queue_mtx);

		}

		pthread_mutex_unlock(&pServer->clients_mtx);
		struct timespec wait_ts;
		if(IsTsNonZero(&min_delay_ts))
		{
			printf("min_delay_ts is nonzero\n");
			wait_ts = min_delay_ts;
		}
		else
		{
			printf("min_delay_ts is zero\n");
			wait_ts.tv_sec = (current_ts.tv_sec + 3600);
			wait_ts.tv_nsec = current_ts.tv_nsec;
		}
		printf("current_ts: %ld ; %ld - wait_ts: %ld; %ld\n",
			current_ts.tv_sec, current_ts.tv_nsec,
			wait_ts.tv_sec, wait_ts.tv_nsec);

		//pthread_mutex_lock(&pThreadData->wakecondmtx);
		pthread_cond_timedwait(&pThreadData->wakecond, &pThreadData->wakecondmtx, &wait_ts);

		//pthread_mutex_unlock(&pThreadData->wakecondmtx);
		printf("WAKING: Wait done at %ld ; %ld\n", wait_ts.tv_sec, wait_ts.tv_nsec);

	}
	printf("Leaving loop\n");
	asThreadCleanup();
	return (void*) 0;
}
