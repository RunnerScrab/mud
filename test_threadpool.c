#include "threadpool.h"
#include "talloc.h"
#include "poolalloc.h"

#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>


struct MemoryPool mempool;


struct Bundle
{
	int v1, v2;
	int* pNum;
	pthread_mutex_t* pMtx;
};

struct ThreadInfo
{
	unsigned long int threadid;
	ssize_t count;
};

struct ThreadInfo* threadinfo = 0;
unsigned int threadcount = 0;


void* TestTask(void* args)
{
	unsigned int i = 0;
	for(; i < threadcount; ++i)
	{
		if(threadinfo[i].threadid == pthread_self())
		{
			++threadinfo[i].count;
			break;
		}
	}
	//	printf("Thread %lld: Test task %d!\n", pthread_self(), ((struct Bundle*) args)->v1);
	struct Bundle* pArgs = (struct Bundle*) args;

	int sum = 0;
	int diff = pArgs->v1;
	for(i = 0; i < 2000000; ++i)
	{
		sum += diff;
	}
	pthread_mutex_lock(pArgs->pMtx);
	*(pArgs->pNum) += sum;
	pthread_mutex_unlock(pArgs->pMtx);
	return 0;
}

void MPoolReleaser(void* args)
{
	MemoryPool_Free(&mempool, sizeof(struct Bundle), args);
}

int main(void)
{
	InitTallocSystem();

	unsigned int cores = get_nprocs() - 1;



	struct ThreadPool tp;
	if(ThreadPool_Init(&tp, cores) < 0)
	{
		printf("Fatal error!\n");
		return -1;
	}
	threadinfo = (struct ThreadInfo*) malloc(sizeof(struct ThreadInfo) * cores);
	memset(threadinfo, 0, sizeof(struct ThreadInfo) * cores);

	unsigned int i = 0;
	threadcount = tp.thread_count;
	for(; i < tp.thread_count; ++i)
	{
		threadinfo[i].threadid = tp.pThreads[i];
		threadinfo[i].count = 0;
	}


	pthread_mutex_t valmtx;
	pthread_mutex_init(&valmtx, 0);


	int poorvalue = 0;

	struct timespec timebegin, timeend;
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timebegin);
	for(; i < 20; ++i)
	{
		//struct Bundle* argbund = talloc(sizeof(struct Bundle));//AllocPool_Alloc(&argpool);
		struct ThreadBundle* tb = ThreadPool_GetLeastBusyThread(&tp);
		printf("Thread_num: %d\n", tb->thread_num);
		struct Bundle* argbund = (struct Bundle*) malloc(sizeof(struct Bundle));

		argbund->pNum = &poorvalue;
		argbund->pMtx = &valmtx;
		argbund->v1 = (i & 1) ? 1 : -1;
		ThreadPool_AddTask(&tp, tb,
				TestTask, 1, argbund, free);

	}
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timeend);
	printf("Computation complete. Result: %d\n", poorvalue);
	printf("Elapsed time: %fs\n", (timeend.tv_nsec - timebegin.tv_nsec)/1000000000.0);


	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timebegin);
	unsigned int j = 0;
	int sum = 0;
	for(poorvalue = 0, i = 0; i < 2000000; ++i)
	{
		for(; j < 2000000; ++j)
		{
			sum += (i & 1) ? 1 : -1;
		}
		poorvalue += sum;
	}
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timeend);
	printf("Computation complete. Result: %d\n", poorvalue);
	printf("Elapsed time for single thread: %fs\n", (timeend.tv_nsec - timebegin.tv_nsec)/1000000000.0);

	ThreadPool_Destroy(&tp);


	for(i = 0; i < threadcount; ++i)
	{
		printf("%lu ran %lu times.\n", threadinfo[i].threadid, threadinfo[i].count);
	}
	scanf("%d", &sum);

	printf("%d outstanding allocations. %d allocs, %d frees.\n", toutstanding_allocs(), tget_allocs(),
		tget_frees());

	StopTallocSystem();
	return 0;
}
