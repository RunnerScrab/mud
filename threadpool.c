#include "threadpool.h"
#include "talloc.h"
#include "heap.h"

#include <sys/sysinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

static void* ThreadPool_WorkerThreadFunc(void* pArg)
{
	struct ThreadBundle* tb = (struct ThreadBundle*) pArg;
	struct HeapNode min;
	while(1)
	{
		for(pthread_mutex_lock(&(tb->my_pq_mtx));
		    0 == Heap_GetSize(&(tb->my_pq)) && tb->bShouldBeRunning;)
		{
			if(0 != pthread_cond_wait(&(tb->my_wakecond), &(tb->my_pq_mtx)))
			{
				//Something screwed up
				pthread_mutex_unlock(&(tb->my_pq_mtx));
				return 0;
			}
		}

		if(!tb->bShouldBeRunning)
		{
			break;
		}


		Heap_ExtractMinimum(&(tb->my_pq), &min);
		// min is now a copy, and the original space on the queue is effectively released


		pthread_mutex_unlock(&(tb->my_pq_mtx));


		struct ThreadTask* pTask = (struct ThreadTask*) min.data;

		if(pTask)
		{
			if(pTask->taskfn)
			{
				pTask->taskfn(pTask->pArgs);
			}

			if(pTask->releasefn)
			{
				//Release task arguments
				pTask->releasefn(pTask->pArgs);
			}

			pthread_mutex_lock(&(tb->mem_pool_mtx));
			MemoryPool_Free(&(tb->mem_pool), sizeof(struct ThreadTask), pTask);
			pthread_mutex_unlock(&(tb->mem_pool_mtx));
			pTask = 0;

		}


	}

	pthread_mutex_unlock(&(tb->my_pq_mtx));
	return 0;
}

struct ThreadBundle* GetThreadBundleByID(struct ThreadPool* tp, unsigned int id)
{
	return &(tp->thread_bundles[id]);
}

unsigned int GetBestThreadID(struct ThreadPool* tp)
{
	//TODO: Make a real load balancing algorithm
	/*
	unsigned int idx = 0;
	int minval = -1;
	for(; idx < tp->thread_count; ++idx)
	{
		int tasks = Heap_GetSize(&(tp->thread_bundles[idx].my_pq));
		minval = (minval == -1 || tasks < minval) ? tasks : minval;
	}
	*/
	unsigned int minval = tp->last_thread_assigned + 1;
	tp->last_thread_assigned = minval % tp->thread_count;
	return minval % tp->thread_count;
}

struct ThreadBundle* ThreadPool_GetLeastBusyThread(struct ThreadPool* tp)
{
	return GetThreadBundleByID(tp, 0);
}

int ThreadBundle_Init(struct ThreadBundle* tb, struct ThreadPool* tp,
		unsigned int thread_num)
{
	memset(tb, 0, sizeof(struct ThreadBundle));
	printf("ThreadBundle Init called!");
	tb->thread_num = thread_num;
	tb->thread_pool = tp;
	tb->bShouldBeRunning = 1;

	if(Heap_Create(&(tb->my_pq), 32) < 0)
		return -1;

	pthread_cond_init(&(tb->my_wakecond), 0);
	pthread_mutex_init(&(tb->my_pq_mtx), 0);

	MemoryPool_Init(&(tb->mem_pool));
	pthread_mutex_init(&(tb->mem_pool_mtx), 0);
	return 0;
}

void ThreadBundle_Destroy(struct ThreadBundle* tb)
{
	pthread_cond_destroy(&(tb->my_wakecond));

	pthread_mutex_lock(&(tb->my_pq_mtx));
	struct ThreadTask* pTask = 0;
	do
	{
		struct HeapNode min;

		Heap_ExtractMinimum(&(tb->my_pq), &min);
		pTask = (struct ThreadTask*) min.data;
		if(pTask && pTask->releasefn)
		{
			pTask->releasefn(pTask->pArgs);
			pTask = 0;
		}
	}
	while(pTask);

	MemoryPool_Destroy(&(tb->mem_pool));
	pthread_mutex_destroy(&(tb->my_pq_mtx));
	pthread_mutex_destroy(&(tb->mem_pool_mtx));
	Heap_Destroy(&(tb->my_pq));
}

int ThreadPool_Init(struct ThreadPool* tp, unsigned int cores)
{
	memset(tp, 0, sizeof(struct ThreadPool));
	unsigned int idx = 0;

	tp->thread_count = cores;

	tp->pThreads = (pthread_t*) talloc(sizeof(pthread_t) * tp->thread_count, __FUNCTION__);
	tp->last_thread_assigned = 0;
	tp->thread_bundles = (struct ThreadBundle*) talloc(sizeof(struct ThreadBundle) * tp->thread_count, __FUNCTION__);

	for(idx = 0; idx < tp->thread_count; ++idx)
	{
		if(ThreadBundle_Init(&(tp->thread_bundles[idx]), tp, idx))
		{
			printf("Thread bundle init failed.\n");
			tfree(tp->thread_bundles);
			return -1;
		}
	}

	for(idx = 0; idx < tp->thread_count; ++idx)
	{
		if(0 != pthread_create(&(tp->pThreads[idx]), 0,
					ThreadPool_WorkerThreadFunc, &(tp->thread_bundles[idx])))
		{
			return -1;
		}
	}

	return 0;
}

void ThreadPool_Stop(struct ThreadPool* tp)
{
	unsigned int idx = 0;
	for(; idx < tp->thread_count; ++idx)
	{
		tp->thread_bundles[idx].bShouldBeRunning = 0;
		pthread_cond_broadcast(&(tp->thread_bundles[idx].my_wakecond));
	}
}

void ThreadPool_Destroy(struct ThreadPool* tp)
{
	ThreadPool_Stop(tp);

	unsigned int idx = 0;
	for(; idx < tp->thread_count; ++idx)
	{
	       	pthread_join(tp->pThreads[idx], 0);

		//TODO: Add some pthread_cancel logic if a thread refuses to join
		//in a timely fashion
	}

	for(idx = 0; idx < tp->thread_count; ++idx)
	{
		ThreadBundle_Destroy(&(tp->thread_bundles[idx]));
	}
	tfree(tp->thread_bundles);
	tfree(tp->pThreads);
	tp->pThreads = 0;
}

int ThreadPool_AddTask(struct ThreadPool* tp, struct ThreadBundle* tb,
		void* (*task) (void*), int priority, void* args,
		void (*argreleaserfn) (void*))
{
	pthread_mutex_lock(&(tb->mem_pool_mtx));
	struct ThreadTask* pTask = (struct ThreadTask*) MemoryPool_Alloc(&(tb->mem_pool), sizeof(struct ThreadTask));
	memset(pTask, 0, sizeof(struct ThreadTask));
	pTask->taskfn = task;
	pTask->releasefn = argreleaserfn;
	pTask->pArgs = args;
	pthread_mutex_unlock(&(tb->mem_pool_mtx));
	pthread_mutex_lock(&(tb->my_pq_mtx));
	int result = Heap_MinInsert(&(tb->my_pq), priority, pTask);
	pthread_mutex_unlock(&(tb->my_pq_mtx));
	pthread_cond_broadcast(&(tb->my_wakecond));
	return result;
}
