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
	struct AllocPool pool;
	AllocPool_Init(&pool, 10, sizeof(struct TestData));
	struct TestData* data[50];
	unsigned int i = 0, j = 0;
        for(j = 0; j < 3; ++j)
	{
		for(i = 0; i < 50; ++i)
		{
			data[i] = AllocPool_Alloc(&pool);
			data[i]->val1 = i;
			data[i]->val2 = i;
			data[i]->val3 = i;
			data[i]->val4 = i;
		}

		for(i = 0; i < 50; ++i)
		{
			printf("%d %d %d %d\n", data[i]->val1,
				data[i]->val2, data[i]->val3, data[i]->val4);
			AllocPool_Free(&pool, data[i]);
		}
	}
	AllocPool_Destroy(&pool);
	printf("%d outstanding allocs\n", toutstanding_allocs());
	return 0;
}
