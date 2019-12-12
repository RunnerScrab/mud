#include "threadpool.h"
#include "talloc.h"
#include "poolalloc.h"

#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

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
	unsigned long long count;
};

struct ThreadInfo* threadinfo = 0;
unsigned int threadcount = 0;

void* TestTask(void* args)
{
	unsigned int i = 0, found = 0;
	for(; i < threadcount; ++i)
	{
		if(threadinfo[i].threadid == pthread_self())
		{
			++threadinfo[i].count;
			found = 1;
			break;
		}
	}
	assert(found == 1);
	//	printf("Thread %lld: Test task %d!\n", pthread_self(), ((struct Bundle*) args)->v1);
	struct Bundle* pArgs = args;

	int sum = 0;
	int diff = pArgs->v1;

	sum += diff;

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
	char ch = 0;
	unsigned int cores = get_nprocs() - 1;



	struct ThreadPool tp;
	if(ThreadPool_Init(&tp, cores) < 0)
	{
		printf("Fatal error!\n");
		return -1;
	}
	threadinfo = (struct ThreadInfo*) malloc(sizeof(struct ThreadInfo) * cores);
	memset(threadinfo, 0, sizeof(struct ThreadInfo) * cores);

	int i = 0;
	threadcount = tp.thread_count;
	for(; i < tp.thread_count; ++i)
	{
		threadinfo[i].threadid = tp.pThreads[i];
		threadinfo[i].count = 0;
	}


	pthread_mutex_t valmtx;
	pthread_mutex_init(&valmtx, 0);
	MemoryPool_Init(&mempool);

	int poorvalue = 0;

	struct timespec timebegin, timeend;
	clock_gettime(CLOCK_PROCESS_CPUTIME_IDk, &timebegin);
	for(; i < 2000000; ++i)
	{
		//struct Bundle* argbund = talloc(sizeof(struct Bundle));//AllocPool_Alloc(&argpool);

		struct Bundle* argbund = MemoryPool_Alloc(&mempool, sizeof(struct Bundle));
		argbund->pNum = &poorvalue;
		argbund->pMtx = &valmtx;
		argbund->v1 = (i & 1) ? 1 : -1;
		ThreadPool_AddTask(&tp, TestTask, 1, argbund, MPoolReleaser);
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
	
	scanf("%d", &sum);
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timeend);
	printf("Computation complete. Result: %d\n", poorvalue);
	printf("Elapsed time for single thread: %fs\n", (timeend.tv_nsec - timebegin.tv_nsec)/1000000000.0);
	
	ThreadPool_Destroy(&tp);


	for(i = 0; i < threadcount; ++i)
	{
		printf("%u ran %llu times.\n", threadinfo[i].threadid, threadinfo[i].count);
	}
	
	MemoryPool_Destroy(&mempool); //lazy; this joins all threads
	printf("%d outstanding allocations. %d allocs, %d frees.\n", toutstanding_allocs(), tget_allocs(),
		tget_frees());


	return 0;
}
