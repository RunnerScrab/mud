#include "threadpool.h"
#include "talloc.h"
#include "poolalloc.h"

#include <stdio.h>
#include <stdlib.h>


struct Bundle
{
	int v1, v2;
	struct AllocPool* ap;
	void* p;
};

void* TestTask(void* args)
{
	printf("Thread %d: Test task %d!\n", pthread_self(), ((struct Bundle*) args)->v1);
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
	struct AllocPool argpool;

	AllocPool_Init(&argpool, 64, sizeof(struct Bundle));
	for(; i < 2000; ++i)
	{

		struct Bundle* argbund = talloc(sizeof(struct Bundle));//AllocPool_Alloc(&argpool);
		argbund->ap = &argpool;
		argbund->v1 = i;

		ThreadPool_AddTask(&tp, TestTask, 1, argbund);
	}

	scanf("%c", &ch);

	AllocPool_Destroy(&argpool);
	ThreadPool_Destroy(&tp);

	printf("%d outstanding allocations. %d allocs, %d frees.\n", toutstanding_allocs(), tget_allocs(),
		tget_frees());
	return 0;
}
