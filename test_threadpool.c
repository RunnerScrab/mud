#include "threadpool.h"
#include "talloc.h"

#include <stdio.h>
#include <stdlib.h>

void* TestTask(void* args)
{
	printf("Test task %d!\n", *((int*)args));
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

	for(; i < 200; ++i)
	{
		int* argint = talloc(sizeof(int));
		*argint = i;
		ThreadPool_AddTask(&tp, TestTask, 1, argint);
	}

	scanf("%c", &ch);

	ThreadPool_Destroy(&tp);

	printf("%d outstanding allocations. %d allocs, %d frees.\n", toutstanding_allocs(), tget_allocs(),
		tget_frees());
	return 0;
}
