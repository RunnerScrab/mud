#include "talloc.h"
#include "heap.h"

#include <sys/sysinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

struct ThreadTask
{
	void* (*taskfn) (void*);
	void* pArgs;

	//If releasefn is supplied, thread will call
	//it on the ThreadTask instance to release
	void* (*releasefn) (void*);
};

struct ThreadPool
{
	pthread_t* pThreads;
	unsigned int thread_count;

	struct Heap prio_queue;
	pthread_mutex_t prio_queue_mutex;
	pthread_cond_t wakecond;

	volatile unsigned char bIsRunning;
};

void* ThreadPool_WorkerThreadFunc(void* pArg)
{
	printf("Thread %d starting\n", pthread_self());
	struct ThreadPool* pPool = (struct ThreadPool*) pArg;

	while(1)
	{

		for(pthread_mutex_lock(&(pPool->prio_queue_mutex));
		    0 == Heap_GetSize(&(pPool->prio_queue)) && pPool->bIsRunning;)
		{
			if(0 != pthread_cond_wait(&(pPool->wakecond), &(pPool->prio_queue_mutex)))
			{
				//Something screwed up
				printf("Fuck\n");
			}
		}

		if(!pPool->bIsRunning)
		{
			break;
		}

		struct HeapNode min;
		int len = Heap_GetSize(&(pPool->prio_queue));
		if(len > 0)
		{
			// min is now a copy, and the original space on the queue is effectively released
			Heap_ExtractMinimum(&(pPool->prio_queue), &min);
			printf("Thread %d - Got task with priority %d!\n", pthread_self(), min.key);
		}


		pthread_mutex_unlock(&(pPool->prio_queue_mutex));


		if(len > 0)
		{
			struct ThreadTask* pTask = (struct ThreadTask*) min.data;

			if(pTask && pTask->taskfn)
			{
				pTask->taskfn(pTask->pArgs);
			}

			if(pTask && pTask->releasefn)
			{
				pTask->releasefn(pTask);
				pTask = 0;
			}
		}

	}
	pthread_mutex_unlock(&(pPool->prio_queue_mutex));
	printf("Thread %d exiting.\n", pthread_self());
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
		//This is not how threads should normally terminate!
	       	pthread_join(tp->pThreads[idx], 0);
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
}

int ThreadPool_Init(struct ThreadPool* tp, unsigned int cores)
{
	int idx = 0;

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


void* TestTask(void* args)
{
	printf("%s", (char*) args);
}

void* TestTaskFree(void* args)
{
	struct ThreadTask* pTask = (struct ThreadTask*) args;
	tfree(pTask->pArgs);
	tfree(pTask);
}


int main(void)
{
	char ch = 0;

	struct ThreadPool tp;
	if(ThreadPool_Init(&tp, get_nprocs()) < 0)
	{
		printf("Fatal error!\n");
		return -1;
	}

	int i = 0;
	pthread_mutex_lock(&(tp.prio_queue_mutex));
	for(; i < 200000; ++i)
	{
		struct ThreadTask* pTask = (struct ThreadTask*) talloc(sizeof(struct ThreadTask));
		pTask->taskfn = TestTask;
		char* message = talloc(sizeof(char) * 32);
		sprintf(message, "Hi from task %d!\n", i);
		pTask->pArgs = message;
		pTask->releasefn = TestTaskFree;


		if(Heap_MinInsert(&(tp.prio_queue), 1, pTask) < 0)
		{
			printf("Allocation FAILED.\n");
			return 0;
		}

	}
	pthread_mutex_unlock(&(tp.prio_queue_mutex));

	scanf("%c", &ch);

	ThreadPool_Destroy(&tp);

	printf("%d outstanding allocations. %d allocs, %d frees.\n", toutstanding_allocs(), tget_allocs(),
		tget_frees());
	return 0;
}
