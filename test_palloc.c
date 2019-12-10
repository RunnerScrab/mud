#include "talloc.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


struct InplaceFreeNode
{
	void* nodedata;
	struct InplaceFreeNode* nextinplacenode;
};

struct AllocPool
{
	struct PoolMemBlock
	{
		void* datablock;
	} *pool_blocks;

	ssize_t element_size, element_count;
	struct InplaceFreeNode* headnode;
	ssize_t block_count;
};

void PoolMemBlock_Init(struct PoolMemBlock* pMemBlock, ssize_t element_count, ssize_t element_size)
{
	pMemBlock->datablock = (void*) talloc(element_count * element_size);

	ssize_t idx = 0;
	struct InplaceFreeNode* pFreeNode = (struct InplaceFreeNode*) pMemBlock->datablock;
	for(; idx < element_count - 1; ++idx)
	{
		pFreeNode->nodedata = pFreeNode;
		pFreeNode->nextinplacenode = pFreeNode + ((idx + 1) * element_size);
		pFreeNode = pFreeNode->nextinplacenode;
	}

	pFreeNode->nodedata = pFreeNode;
	pFreeNode->nextinplacenode = ((void*) 0);
}

void AllocPool_Init(struct AllocPool* pAllocPool, ssize_t element_count,
		ssize_t element_size)
{
	pAllocPool->element_size = element_size;
	pAllocPool->element_count = element_count;
	pAllocPool->pool_blocks = (struct PoolMemBlock*) talloc(sizeof(struct PoolMemBlock));
	PoolMemBlock_Init(pAllocPool->pool_blocks, element_count, element_size);
	pAllocPool->block_count = 1;
	pAllocPool->headnode = pAllocPool->pool_blocks[0].datablock;
}

void AllocPool_AddBlock(struct AllocPool* pAllocPool)
{
	if(!pAllocPool->headnode)
	{
		pAllocPool->pool_blocks = (struct PoolMemBlock*) realloc(pAllocPool->pool_blocks,
									(pAllocPool->block_count + 1) *
									sizeof(struct PoolMemBlock));
		PoolMemBlock_Init(&pAllocPool->pool_blocks[pAllocPool->block_count],
				pAllocPool->element_count, pAllocPool->element_size);
		++(pAllocPool->block_count);

		pAllocPool->headnode = pAllocPool->pool_blocks[pAllocPool->block_count - 1].datablock;
	}
}

void* AllocPool_Alloc(struct AllocPool* pAllocPool)
{
	struct InplaceFreeNode* returnval = pAllocPool->headnode;
	pAllocPool->headnode = returnval->nextinplacenode;
	if(!pAllocPool->headnode)
	{
		AllocPool_AddBlock(pAllocPool);
	}
	return returnval->nodedata;
}

void AllocPool_Free(struct AllocPool* pAllocPool, void* pFreeMe)
{
	//Once pFreeMe has been used (which it must be assumed to have been),
	//the nodedata and nextinplacenode have already been overwritten with data
	((struct InplaceFreeNode*)pFreeMe)->nextinplacenode = pAllocPool->headnode;
	((struct InplaceFreeNode*)pFreeMe)->nodedata = pFreeMe;
	pAllocPool->headnode = pFreeMe;
}

void AllocPool_Destroy(struct AllocPool* pAllocPool)
{
	ssize_t idx = 0;
	for(; idx < pAllocPool->block_count; ++idx)
	{
		tfree(pAllocPool->pool_blocks[idx].datablock);
	}
	tfree(pAllocPool->pool_blocks);
}


struct TestData
{
	int val1, val2, val3, val4;
};

int main(void)
{
	struct AllocPool pool;
	AllocPool_Init(&pool, 10, sizeof(struct TestData));
	struct TestData* data[20];
	unsigned int i = 0, j = 0;
        for(j = 0; j < 3; ++j)
	{
		for(i = 0; i < 20; ++i)
		{
			data[i] = AllocPool_Alloc(&pool);
			data[i]->val1 = i;
			data[i]->val2 = i;
			data[i]->val3 = i;
			data[i]->val4 = i;
		}

		for(i = 0; i < 20; ++i)
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
