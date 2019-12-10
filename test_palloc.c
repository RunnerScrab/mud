#include "talloc.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


struct FreeNode
{
	void* pFreeData;
	struct FreeNode* next;
};

struct AllocPool
{
	void* pPool;
	ssize_t block_size;
	ssize_t elements;

	struct FreeNode* pHead;
};

void Pool_Init(struct AllocPool* pool, ssize_t elements, ssize_t element_size)
{
	int idx = 0;
	void* pBase = 0;
	pool->pPool = (void*) talloc(elements * element_size);
	pool->elements = elements;
	pool->block_size = element_size;
	pBase = pool->pPool;

	memset(pool->pPool, 0, elements * element_size);
	struct FreeNode* p = pBase;
	pool->pHead = pBase;
	printf("%x\n", p);
	for(;idx < elements - 1; ++idx)
	{
		p->pFreeData = ((struct FreeNode*) pBase + (idx * element_size));
	        p->next = ((struct FreeNode*) pBase + ((idx + 1) * element_size));
		p = p->next;

	}

	p->pFreeData = p;
	p->next = 0;
	pool->pHead = pool->pPool;
	p = pool->pHead;

	for(; p; p = p->next)
	{
		printf("%x, %x\n", p->pFreeData, p->next);

	}


}

void* Pool_Alloc(struct AllocPool* pool)
{
	void* returnme = pool->pHead->pFreeData;
	pool->pHead = ((struct FreeNode*)returnme)->next;
	return returnme;
}

void Pool_Destroy(struct AllocPool* pool)
{
  tfree(pool->pPool);
  pool->pPool = 0;
  pool->pHead = 0;
}

void Pool_Free(struct AllocPool* pool, void* pFreeMe)
{
  ((struct FreeNode*) pFreeMe)->pFreeData = pFreeMe;
  ((struct FreeNode*) pFreeMe)->next = pool->pHead;
  pool->pHead = pFreeMe;
}

struct TestDatum
{
	int value1, value2, value3, value4;
};


int main(void)
{
	printf("Size of TestDatum: %d\nSize of FreeNode: %d\n",
		sizeof(struct TestDatum), sizeof(struct FreeNode));
	struct AllocPool pool;
	Pool_Init(&pool, 10, sizeof(struct TestDatum));
	int i = 0, j = 0;
	struct TestDatum* data[10];
	for(; j < 2; ++j)
	  {
	for(i = 0; i < 10; ++i)
	  {
	    data[i] = Pool_Alloc(&pool);
	    data[i]->value1 = 1 + i;
	    data[i]->value2 = 2 + i;
	    data[i]->value3 = 3 + i;
	    data[i]->value4 = 4 + i;
	  }
	for(i = 0; i < 10; ++i)
	  {
	    printf("%d %d %d %d\n", data[i]->value1,
		   data[i]->value2,
		   data[i]->value3,
		   data[i]->value4);
	    Pool_Free(&pool, data[i]);
	  }

	  }

	Pool_Destroy(&pool);
	printf("%d outstanding allocations.\n", toutstanding_allocs());
	return 0;
}
