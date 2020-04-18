#include "action_scheduler.h"

#include "utils.h"
extern "C"
{
#include "server.h"
#include "hrt_prioq.h"
#include "threadpool.h"
#include "as_cinterface.h"
}

#include <sys/time.h>
#include <pthread.h>
#include <exception>

ActionScheduler::ActionScheduler(struct Server *server) :
		m_thread()
{
	pthread_rwlock_init(&m_active_actors_rwlock, 0);

	m_pServer = server;
	m_bIsRunning = 1;

	pthread_condattr_init(&m_wakecondattr);
	if (pthread_condattr_setclock(&m_wakecondattr, CLOCK_MONOTONIC) < 0)
	{
		ServerLog(SERVERLOG_ERROR, "Failed to set cond clock attribute!");
		pthread_condattr_destroy(&m_wakecondattr);
		pthread_rwlock_destroy(&m_active_actors_rwlock);
		throw std::exception();
	}
	pthread_cond_init(&m_wakecond, &m_wakecondattr);
	pthread_mutex_init(&m_wakecondmtx, 0);
}

ActionScheduler::~ActionScheduler()
{
	pthread_rwlock_wrlock(&m_active_actors_rwlock);
	pthread_rwlock_destroy(&m_active_actors_rwlock);
	pthread_cond_destroy(&m_wakecond);
	pthread_condattr_destroy(&m_wakecondattr);
	pthread_mutex_destroy(&m_wakecondmtx);
}

void ActionScheduler::StartThread(void)
{
	pthread_create(&m_thread, 0, ActionScheduler::ThreadFunction, (void*) this);
}

void ActionScheduler::StopThread(void)
{
	if (m_bIsRunning)
	{
		ServerLog(SERVERLOG_STATUS, "Stopping action scheduler thread.");
		m_bIsRunning = 0;
		pthread_cond_broadcast(&m_wakecond);
		ServerLog(SERVERLOG_STATUS, "Wake broadcast");
		pthread_join(m_thread, 0);
		ServerLog(SERVERLOG_STATUS, "Action scheduler thread joined.");
	}
}

void ActionScheduler::AddActiveActor(NativeActor *actor)
{

	pthread_rwlock_wrlock(&m_active_actors_rwlock);
	if (m_active_actors.find(actor) == m_active_actors.end())
	{
		m_active_actors.insert(actor);
		actor->AddRef();
	}
	pthread_rwlock_unlock(&m_active_actors_rwlock);
}

static inline void ZeroTs(struct timespec *ts)
{
	ts->tv_sec = 0;
	ts->tv_nsec = 0;
}

int IsTsNonZero(struct timespec *ts)
{
	return ts->tv_sec && ts->tv_nsec;
}

void ActionScheduler::RemoveActor(NativeActor *actor)
{
	pthread_rwlock_wrlock(&m_active_actors_rwlock);
	m_active_actors.erase(m_active_actors.find(actor));
	pthread_rwlock_unlock(&m_active_actors_rwlock);
	actor->Release();
}

void* ActionScheduler::ThreadFunction(void *pArgs)
{
	ServerLog(SERVERLOG_STATUS, "Running user command dispatch thread");
	ActionScheduler *pActionScheduler = (struct ActionScheduler*) pArgs;
	struct Server *pServer = pActionScheduler->m_pServer;

	struct timespec current_ts, delay_ts, min_delay_ts;

	while (pActionScheduler->m_bIsRunning)
	{
		if (clock_gettime(CLOCK_MONOTONIC, &current_ts) < 0)
		{
			ServerLog(SERVERLOG_ERROR,
					"Failed to get current timestamp from system clock.");
			continue;
		}

		pthread_rwlock_wrlock(&pActionScheduler->m_active_actors_rwlock);
		ZeroTs(&min_delay_ts);

		for (std::set<NativeActor*>::iterator it =
				pActionScheduler->m_active_actors.begin();
				it != pActionScheduler->m_active_actors.end();)
		{
			NativeActor *pActor = *it;
			pActor->LockQueue();

			struct hrt_prioq *aqueue = pActor->GetQueue();

			while (hrt_prioq_get_size(aqueue) > 0)
			{
				ZeroTs(&delay_ts);
				hrt_prioq_get_key_at(aqueue, 0, &delay_ts);
				if (CmpTs(&current_ts, &delay_ts) >= 0)
				{
					struct ThreadTask *pTask =
							(struct ThreadTask*) hrt_prioq_extract_min(aqueue);
					dbgprintf("Putting task in threadpool.\n");
					ThreadPool_AddTask(&pServer->thread_pool, pTask->taskfn, 0,
							pTask->pArgs, pTask->releasefn);
					pActor->FreeQueueTask(pTask);
				}
				else
				{
					dbgprintf("Found event for later.\n");
					if (!IsTsNonZero(&min_delay_ts)
							|| (IsTsNonZero(&delay_ts)
									&& CmpTs(&min_delay_ts, &delay_ts) == 1))
					{
						min_delay_ts.tv_sec = delay_ts.tv_sec + 1;
						min_delay_ts.tv_nsec = delay_ts.tv_nsec;
					}
					break;
				}
			}

			if (!hrt_prioq_get_size(aqueue))
			{
				pActor->UnlockQueue();
				pActor->Release();
				std::set<NativeActor*>::iterator erasethis = it;
				++it;
				pActionScheduler->m_active_actors.erase(erasethis);
			}
			else
			{
				pActor->UnlockQueue();
				++it;
			}

		}
		pthread_rwlock_unlock(&pActionScheduler->m_active_actors_rwlock);

		struct timespec wait_ts;
		if (IsTsNonZero(&min_delay_ts))
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

		dbgprintf("current_ts: %ld ; %ld - wait_ts: %ld; %ld\n",
				current_ts.tv_sec, current_ts.tv_nsec, wait_ts.tv_sec,
				wait_ts.tv_nsec);

		pthread_cond_timedwait(&pActionScheduler->m_wakecond,
				&pActionScheduler->m_wakecondmtx, &wait_ts);

		dbgprintf("WAKING: Wait done at %ld ; %ld\n", wait_ts.tv_sec,
				wait_ts.tv_nsec);

	}

	//Release all references to the actors in our set, so ARC
	//can free them
	for (NativeActor *pActor : pActionScheduler->m_active_actors)
	{
		pActor->Release();
	}

	ServerLog(SERVERLOG_STATUS, "Action scheduler thread exiting.");
	CCompatibleASThreadCleanup();
	return (void*) 0;
}
