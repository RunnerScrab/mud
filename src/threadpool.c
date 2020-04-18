#include "threadpool.h"
#include "talloc.h"
#include "prioq.h"

#include "as_cinterface.h"
#include <sys/sysinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#ifdef __cplusplus
extern "C" {
#endif

static void* ThreadPool_WorkerThreadFunc(void *pArg)
{
	struct ThreadPool *pPool = (struct ThreadPool*) pArg;

	while (1)
	{
		for (pthread_mutex_lock(&pPool->prio_queue_mutex);
				0 == prioq_get_size(&pPool->prio_queue) && pPool->bIsRunning;)
		{
			if (0
					!= pthread_cond_wait(&pPool->wakecond,
							&pPool->prio_queue_mutex))
			{
				//Something screwed up
				pthread_mutex_unlock(&pPool->prio_queue_mutex);
				return 0;
			}
		}

		if (!pPool->bIsRunning)
		{
			break;
		}

		// min is now a copy, and the original space on the queue is effectively
		// released

		//Retrieve the next task from the priority queue
		struct ThreadTask *pTask = (struct ThreadTask*) prioq_extract_min(
				&pPool->prio_queue);
		pthread_mutex_unlock(&pPool->prio_queue_mutex);

		if (pTask)
		{
			if (pTask->taskfn)
			{
				//Run the task
				pTask->taskfn(pTask->pArgs);
			}

			if (pTask->releasefn)
			{
				//Release task arguments using the free function
				//passed to the task, if we've been provided one
				pTask->releasefn(pTask->pArgs);

			}

			//Data about the task itself (not its args,
			//necessarily) are always allocated from a
			//preallocated pool to avoid context switching
			//to ask the system for memory every time one
			//is to be dispatched

			AllocPool_Free(&pPool->alloc_pool, pTask);
			pTask = 0;
		}

	}
	//This unlock is normally unnecessary and is for if we need to exit the
	//worker thread loop suddenly
	CCompatibleASThreadCleanup();
	pthread_mutex_unlock(&pPool->prio_queue_mutex);
	return 0;
}

void ThreadPool_Stop(struct ThreadPool *tp)
{
	tp->bIsRunning = 0;
	pthread_cond_broadcast(&tp->wakecond);
}

void ThreadPool_Destroy(struct ThreadPool *tp)
{
	ThreadPool_Stop(tp);

	size_t idx = 0;
	for (; idx < tp->thread_count; ++idx)
	{
		pthread_join(tp->pThreads[idx], 0);
		//TODO: Add some pthread_cancel logic if a thread refuses to join
		//in a timely fashion
	}

	//Burn through the remaining tasks enqueued so that we release all resources
	pthread_mutex_lock(&tp->prio_queue_mutex);
	struct ThreadTask *pTask = 0;
	do
	{
		pTask = (struct ThreadTask*) prioq_extract_min(&tp->prio_queue);

		if (pTask && pTask->releasefn)
		{
			pTask->releasefn(pTask);
			pTask = 0;
		}
	} while (pTask);

	prioq_destroy(&tp->prio_queue);
	pthread_cond_destroy(&tp->wakecond);
	pthread_mutex_destroy(&tp->prio_queue_mutex);

	tfree(tp->pThreads);
	tp->pThreads = 0;

	AllocPool_Destroy(&tp->alloc_pool);
}

int ThreadPool_Init(struct ThreadPool *tp, unsigned int cores)
{
	size_t idx = 0;

	AllocPool_Init(&(tp->alloc_pool), 64, sizeof(struct ThreadTask));

	tp->thread_count = cores; // - 1 for the server thread?
	tp->pThreads = (pthread_t*) talloc(sizeof(pthread_t) * tp->thread_count);

	if (pthread_cond_init(&(tp->wakecond), 0) < 0)
	{
		return -1;
	}

	if (pthread_mutex_init(&(tp->prio_queue_mutex), 0) < 0)
	{
		return -1;
	}

	if (prioq_create(&(tp->prio_queue), 256) < 0)
	{
		return -1;
	}

	tp->bIsRunning = 1;

	for (; idx < tp->thread_count; ++idx)
	{
		if (0
				!= pthread_create(&(tp->pThreads[idx]), 0,
						ThreadPool_WorkerThreadFunc, tp))
		{
			return -1;
		}
	}

	return 0;
}

struct ThreadTask* CreateTask(struct ThreadPool *tp, void* (*taskfn)(void*),
		int priority, void *args, void (*argreleaserfn)(void*))
{
	struct ThreadTask *pTask = (struct ThreadTask*) AllocPool_Alloc(
			&tp->alloc_pool);
	pTask->taskfn = taskfn;
	pTask->pArgs = args;
	pTask->releasefn = argreleaserfn;
	return pTask;
}

//args should be a pointer to allocated memory; the threadpool takes ownership
int ThreadPool_AddTask(struct ThreadPool *tp, void* (*task)(void*),
		int priority, void *args, void (*argreleaserfn)(void*))
{
	struct ThreadTask *pTask = CreateTask(tp, task, priority, args,
			argreleaserfn);
	int insresult = 0;

	pthread_mutex_lock(&tp->prio_queue_mutex);

	insresult = prioq_min_insert(&tp->prio_queue, priority, pTask);

	pthread_mutex_unlock(&tp->prio_queue_mutex);
	pthread_cond_signal(&tp->wakecond);

	return insresult;
}

#ifdef __cplusplus
}
#endif

