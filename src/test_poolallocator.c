#include "talloc.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "poolalloc.h"

struct TestData
{
	int val1, val2, val3, val4;
};

int main(void)
{
	struct MemoryPool mempool;
	MemoryPool_Init(&mempool);
	struct TestData* data[100];
	unsigned int i = 0, j = 0;
        for(j = 0; j < 3; ++j)
	{
		for(i = 0; i < 100; ++i)
		{
			data[i] = MemoryPool_Alloc(&mempool, sizeof(struct TestData));
			data[i]->val1 = i;
			data[i]->val2 = i;
			data[i]->val3 = i;
			data[i]->val4 = i;
		}

		for(i = 0; i < 100; ++i)
		{
			printf("%d %d %d %d\n", data[i]->val1,
				data[i]->val2, data[i]->val3, data[i]->val4);
			MemoryPool_Free(&mempool, sizeof(struct TestData), data[i]);
		}
	}
	MemoryPool_Destroy(&mempool);
	printf("%d outstanding allocs\n", toutstanding_allocs());
	return 0;
}
