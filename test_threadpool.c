#include "threadpool.h"
#include "talloc.h"
#include "poolalloc.h"
#include <mcheck.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>





struct Bundle
{
	int v1, v2;
	int* pNum;
	pthread_mutex_t* pMtx;
};



void* TestTask(void* args)
{
	printf("Test task %d!\n", ((struct Bundle*) args)->v1);
	struct Bundle* pArgs = args;
	unsigned int i = 0;
	int sum = 0;
	int diff = pArgs->v1;
	for(; i < 2000000; ++i)
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
//	MemoryPool_Free(&mempool, sizeof(struct Bundle), args);
}

int main(void)
{
	mtrace();
	struct MemoryPool mempool;
	memset(&mempool, 0, sizeof(struct MemoryPool));
	struct ThreadPool tp;
	if(ThreadPool_Init(&tp, get_nprocs()) < 0)
	{
		printf("Fatal error!\n");
		return -1;
	}

	int i = 0;
	pthread_mutex_t valmtx;
	pthread_mutex_init(&valmtx, 0);
	MemoryPool_Init(&mempool);

	int poorvalue = 0;

	struct timespec timebegin, timeend;
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timebegin);
	for(; i < 200; ++i)
	{
		//struct Bundle* argbund = talloc(sizeof(struct Bundle));//AllocPool_Alloc(&argpool);

		struct Bundle* argbund = MemoryPool_Alloc(&mempool, sizeof(struct Bundle));
		argbund->pNum = &poorvalue;
		argbund->pMtx = &valmtx;
		argbund->v1 = (i & 1) ? 1 : -1;
		ThreadPool_AddTask(&tp, ThreadPool_GetLeastBusyThread(&tp),
				TestTask, 1, argbund, MPoolReleaser);

	}
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timeend);
	printf("Computation complete. Result: %d\n", poorvalue);
	printf("Elapsed time: %fs\n", (timeend.tv_nsec - timebegin.tv_nsec)/1000000000.0);


	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timebegin);
	unsigned int j = 0;
	int sum = 0;
	for(poorvalue = 0, i = 0; i < 200; ++i)
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

	scanf("%d", &sum);
	MemoryPool_Destroy(&mempool); //lazy; this joins all threads
	ThreadPool_Destroy(&tp);

	printf("%d outstanding allocations. %d allocs, %d frees.\n", toutstanding_allocs(), tget_allocs(),
		tget_frees());
	muntrace();
	return 0;
}
