#include "threadpool.h"
#include "talloc.h"
#include "poolalloc.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>


struct MemoryPool mempool;
pthread_mutex_t mempool_mtx;

struct Bundle
{
	int v1, v2;
	void* p;
};

void* TestTask(void* args)
{
	pthread_mutex_lock(&mempool_mtx);
	printf("Thread %lld: Test task %d!\n", pthread_self(), ((struct Bundle*) args)->v1);
	MemoryPool_Free(&mempool, sizeof(struct Bundle), args);
	pthread_mutex_unlock(&mempool_mtx);
}

void MPoolReleaser(void* args)
{
	pthread_mutex_lock(&mempool_mtx);
	MemoryPool_Free(&mempool, sizeof(struct Bundle), args);
	pthread_mutex_unlock(&mempool_mtx);
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

	pthread_mutex_init(&mempool_mtx, 0);
	MemoryPool_Init(&mempool);

	for(; i < 200; ++i)
	{
		//struct Bundle* argbund = talloc(sizeof(struct Bundle));//AllocPool_Alloc(&argpool);
		pthread_mutex_lock(&mempool_mtx);
		struct Bundle* argbund = MemoryPool_Alloc(&mempool, sizeof(struct Bundle));
		argbund->v1 = i;
		pthread_mutex_unlock(&mempool_mtx);
		ThreadPool_AddTask(&tp, TestTask, 1, argbund, MPoolReleaser);
	}

	scanf("%c", &ch);

	pthread_mutex_lock(&mempool_mtx);
	MemoryPool_Destroy(&mempool);
	pthread_mutex_destroy(&mempool_mtx);
	ThreadPool_Destroy(&tp);

	printf("%d outstanding allocations. %d allocs, %d frees.\n", toutstanding_allocs(), tget_allocs(),
		tget_frees());
	return 0;
}
