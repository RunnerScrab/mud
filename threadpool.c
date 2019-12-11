#include "threadpool.h"
#include "talloc.h"
#include "heap.h"

#include <sys/sysinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

static void* ThreadPool_WorkerThreadFunc(void* pArg)
{
	struct ThreadPool* pPool = (struct ThreadPool*) pArg;
	struct HeapNode min;
	while(1)
	{
		for(pthread_mutex_lock(&(pPool->prio_queue_mutex));
		    0 == Heap_GetSize(&(pPool->prio_queue)) && pPool->bIsRunning;)
		{
			if(0 != pthread_cond_wait(&(pPool->wakecond), &(pPool->prio_queue_mutex)))
			{
				//Something screwed up
				pthread_mutex_unlock(&(pPool->prio_queue_mutex));
				return 0;
			}
		}

		if(!pPool->bIsRunning)
		{
			break;
		}


		Heap_ExtractMinimum(&(pPool->prio_queue), &min);
		// min is now a copy, and the original space on the queue is effectively released


		pthread_mutex_unlock(&(pPool->prio_queue_mutex));


		struct ThreadTask* pTask = (struct ThreadTask*) min.data;

		if(pTask)
		{
			if(pTask->taskfn)
			{
				pTask->taskfn(pTask->pArgs);
			}

			if(pTask->releasefn)
			{
				pTask->releasefn(pTask);
				AllocPool_Free(&(pPool->alloc_pool), pTask);
				pTask = 0;
			}
		}


	}

	pthread_mutex_unlock(&(pPool->prio_queue_mutex));
	return 0;
}

void ThreadPool_Stop(struct ThreadPool* tp)
{
	tp->bIsRunning = 0;
	pthread_cond_broadcast(&(tp->wakecond));
}

void ThreadPool_Destroy(struct ThreadPool* tp)
{
	ThreadPool_Stop(tp);

	int idx = 0;
	for(; idx < tp->thread_count; ++idx)
	{
	       	pthread_join(tp->pThreads[idx], 0);
		//TODO: Add some pthread_cancel logic if a thread refuses to join
		//in a timely fashion
	}

	pthread_mutex_lock(&(tp->prio_queue_mutex));
	struct ThreadTask* pTask = 0;
	do
	{
		struct HeapNode min;

		Heap_ExtractMinimum(&(tp->prio_queue), &min);
		pTask = min.data;
		if(pTask && pTask->releasefn)
		{
			pTask->releasefn(pTask);
			pTask = 0;
		}
	}
	while(pTask);
	Heap_Destroy(&(tp->prio_queue));
	pthread_cond_destroy(&(tp->wakecond));
	pthread_mutex_destroy(&(tp->prio_queue_mutex));

	tfree(tp->pThreads);
	tp->pThreads = 0;

	AllocPool_Destroy(&(tp->alloc_pool));
}

int ThreadPool_Init(struct ThreadPool* tp, unsigned int cores)
{
	int idx = 0;

	AllocPool_Init(&(tp->alloc_pool), 64, sizeof(struct ThreadTask));

	tp->thread_count = cores; // - 1 for the server thread?
	tp->pThreads = (pthread_t*) talloc(sizeof(pthread_t) * tp->thread_count);

	if(pthread_cond_init(&(tp->wakecond), 0) < 0)
	{
		return -1;
	}

	if(pthread_mutex_init(&(tp->prio_queue_mutex), 0) < 0)
	{
		return -1;
	}

	if(Heap_Create(&(tp->prio_queue), 256) < 0)
	{
		return -1;
	}

	tp->bIsRunning = 1;

	for(; idx < tp->thread_count; ++idx)
	{
		if(0 != pthread_create(&(tp->pThreads[idx]), 0,
					ThreadPool_WorkerThreadFunc, tp))
		{
			return -1;
		}
	}

	return 0;
}


static void* DefaultTaskFree(void* args)
{
	struct ThreadTask* pTask = (struct ThreadTask*) args;
	tfree(pTask->pArgs);
}

//args should be a pointer to allocated memory; the threadpool takes ownership
int ThreadPool_AddTask(struct ThreadPool* tp, void* (*task) (void*), int priority, void* args)
{
	struct ThreadTask* pTask = (struct ThreadTask*) AllocPool_Alloc(&(tp->alloc_pool));

	pTask->taskfn = task;
	pTask->pArgs = args;
	pTask->releasefn = DefaultTaskFree;

	int insresult = 0;
	pthread_mutex_lock(&(tp->prio_queue_mutex));
	insresult = Heap_MinInsert(&(tp->prio_queue), priority, pTask);
	pthread_mutex_unlock(&(tp->prio_queue_mutex));
	pthread_cond_signal(&(tp->wakecond));

	return insresult;
}
