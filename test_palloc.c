#include "talloc.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


struct FreeNode
{
	void* pFreeData;
	struct FreeNode* pNext;
};

struct AllocPool
{
	struct FreeNode* pData;
	ssize_t element_size;
	ssize_t elements;

	struct FreeNode* pHead;
};

void Pool_InitFreeData(struct FreeNode* p, ssize_t elements, ssize_t element_size)
{
	/*
	 p points to the root node in the linked list, which is to be
	 allocated /in/ the memory of the free pool
	*/
	ssize_t idx = 0;
	struct FreeNode* pBase = p;

	for(;idx < elements - 1; ++idx)
	{
		p->pFreeData = ((struct FreeNode*) pBase
				+ (idx * element_size));
	        p->pNext = ((struct FreeNode*) pBase
			+ ((idx + 1) * element_size));
		p = p->pNext;
	}

	p->pFreeData = p;
	p->pNext = 0ull;
}

void Pool_Init(struct AllocPool* pool, ssize_t elements, ssize_t element_size)
{
	pool->pData = (struct FreeNode*) talloc(sizeof(struct FreeNode));

	pool->pData->pFreeData = (void*) talloc(elements * element_size);
	pool->pData->pNext = 0;

	pool->elements = elements;
	pool->element_size = element_size;

	pool->pHead = pool->pData->pFreeData;

	Pool_InitFreeData(pool->pHead, elements, element_size);
}

void* Pool_ItemAlloc(struct AllocPool* pool)
{
	if(pool->pHead)
	{
		void* returnme = pool->pHead->pFreeData;
		pool->pHead = pool->pHead->pNext;
		return returnme;
	}
	else
	{
		//If we have run out of free nodes, allocate a new data block
		struct FreeNode** pBlock = &(pool->pData);
		for(; *pBlock; pBlock = &((*pBlock)->pNext));
		*pBlock = (struct FreeNode*) talloc(sizeof(struct FreeNode));
		(*pBlock)->pNext = 0;
		(*pBlock)->pFreeData = (void*) talloc(pool->elements * 2 * pool->element_size);

		pool->pHead = (*pBlock)->pFreeData;
		Pool_InitFreeData(pool->pHead, pool->elements * 2, pool->element_size);
		pool->elements += pool->elements * 2;

		return Pool_ItemAlloc(pool);
	}

}

void Pool_Destroy(struct AllocPool* pool)
{
	struct FreeNode* p = pool->pData, *t = 0;
	for(;p;)
	{
		t = p->pNext;
		tfree(p->pFreeData);
		tfree(p);
		p = t;
	}

	pool->pData = 0;
	pool->pHead = 0;
}

void Pool_ItemFree(struct AllocPool* pool, void* pFreeMe)
{
	if(pFreeMe)
	{
		((struct FreeNode*) pFreeMe)->pFreeData = pFreeMe;
		((struct FreeNode*) pFreeMe)->pNext = pool->pHead;
		pool->pHead = pFreeMe;
	}
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
	struct TestDatum* data[20];
	for(; j < 2; ++j)
	{
		for(i = 0; i < 20; ++i)
		{
			data[i] = Pool_ItemAlloc(&pool);
			data[i]->value1 = 1 + i*2;
			data[i]->value2 = 2 + i*2;
			data[i]->value3 = 3 + i*2;
			data[i]->value4 = 4 + i*2;
		}
		for(i = 0; i < 20; ++i)
		{
			printf("%d %d %d %d\n", data[i]->value1,
				data[i]->value2,
				data[i]->value3,
				data[i]->value4);
			Pool_ItemFree(&pool, data[i]);
		}

	}

	Pool_Destroy(&pool);
	printf("%d outstanding allocations.\n", toutstanding_allocs());
	return 0;
}
