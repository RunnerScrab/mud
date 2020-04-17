#include "command_dispatch.h"
#include "utils.h"
#include "server.h"
#include "as_cinterface.h"
#include <sys/time.h>

int CmdDispatchThread_Init(struct CmdDispatchThread* dispatchthread, struct Server* server)
{
	Vector_Create(&dispatchthread->active_actors, 64, 0);
	pthread_mutex_init(&dispatchthread->active_actors_mtx, 0);

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
	if(dispatchthread->bIsRunning)
	{
		ServerLog(SERVERLOG_STATUS, "Stopping cmd dispatch thread.");
		dispatchthread->bIsRunning = 0;
		pthread_cond_broadcast(&dispatchthread->wakecond);
		ServerLog(SERVERLOG_STATUS, "Wake broadcast");
		pthread_join(dispatchthread->thread, 0);
		ServerLog(SERVERLOG_STATUS, "Cmd dispatch thread joined.");
	}
}

void CmdDispatchThread_Destroy(struct CmdDispatchThread* dispatchthread)
{
	pthread_mutex_lock(&dispatchthread->active_actors_mtx);
	Vector_Destroy(&dispatchthread->active_actors);
	pthread_mutex_destroy(&dispatchthread->active_actors_mtx);

	pthread_cond_destroy(&dispatchthread->wakecond);
	pthread_condattr_destroy(&dispatchthread->wakecondattr);
	pthread_mutex_destroy(&dispatchthread->wakecondmtx);
}

void ZeroTs(struct timespec* ts)
{
	ts->tv_sec = 0;
	ts->tv_nsec = 0;
}

int IsTsNonZero(struct timespec* ts)
{
	return ts->tv_sec && ts->tv_nsec;
}

void CmdDispatchThread_AddActiveActor(struct CmdDispatchThread* dispatchthread, struct Actor* actor)
{
	pthread_mutex_lock(&dispatchthread->active_actors_mtx);
	Vector_Push(&dispatchthread->active_actors, actor);
	pthread_mutex_unlock(&dispatchthread->active_actors_mtx);
	Actor_AddRef(actor);
}

inline static int CmpActor(void* a, void* b)
{
	return a - b;
}

void CmdDispatchThread_RemoveActor(struct CmdDispatchThread* dispatchthread, struct Actor* actor)
{
	size_t result_idx = 0;
	pthread_mutex_lock(&dispatchthread->active_actors_mtx);
	if(!Vector_Find(&dispatchthread->active_actors, actor, CmpActor, &result_idx))
	{
		Vector_Remove(&dispatchthread->active_actors, result_idx);
	}
	pthread_mutex_unlock(&dispatchthread->active_actors_mtx);

	Actor_ReleaseRef(actor);
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

		pthread_mutex_lock(&pThreadData->active_actors_mtx);
		ZeroTs(&min_delay_ts);

		for(idx = 0, len = Vector_Count(&pThreadData->active_actors);
		    idx < len; ++idx)
		{
			struct Actor* pActor = (struct Actor*) Vector_At(&pThreadData->active_actors, idx);
			pthread_mutex_lock(&pActor->action_queue_mutex);
			ZeroTs(&delay_ts);
			while(hrt_prioq_get_size(&pActor->action_queue) > 0)
			{
				hrt_prioq_get_key_at(&pActor->action_queue, 0, &delay_ts);
				if(CmpTs(&current_ts, &delay_ts) >= 0)
				{
					struct ThreadTask* pTask = (struct ThreadTask*) hrt_prioq_extract_min(&pActor->action_queue);
					ThreadPool_AddTask(&pServer->thread_pool, pTask->taskfn, 0,
							pTask->pArgs, pTask->releasefn);
					MemoryPool_Free(&pActor->mem_pool, sizeof(struct ThreadTask), pTask);

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

			pthread_mutex_unlock(&pActor->action_queue_mutex);

			if(!hrt_prioq_get_size(&pActor->action_queue))
			{
				CmdDispatchThread_RemoveActor(pThreadData, pActor);
				--len;
				--idx;
			}

		}
		pthread_mutex_unlock(&pThreadData->active_actors_mtx);

		struct timespec wait_ts;
		if(IsTsNonZero(&min_delay_ts))
		{
			wait_ts = min_delay_ts;
		}
		else
		{
			//We should wait indefinitely if there are no user commands queued,
			//but an hour is just as good in CPU thread time
			wait_ts.tv_sec = (current_ts.tv_sec + 3600);
			wait_ts.tv_nsec = current_ts.tv_nsec;
		}

		  dbgprintf("current_ts: %ld ; %ld - wait_ts: %ld; %ld\n", current_ts.tv_sec,
			current_ts.tv_nsec, wait_ts.tv_sec, wait_ts.tv_nsec);


		pthread_cond_timedwait(&pThreadData->wakecond, &pThreadData->wakecondmtx, &wait_ts);

		dbgprintf("WAKING: Wait done at %ld ; %ld\n", wait_ts.tv_sec, wait_ts.tv_nsec);

	}
	ServerLog(SERVERLOG_STATUS, "Command Dispatch thread exiting.");
	CCompatibleASThreadCleanup();
	return (void*) 0;
}
