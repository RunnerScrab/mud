#include "poolalloc.h"
#include "talloc.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static ssize_t np2(ssize_t num, ssize_t multiple)
{
	ssize_t multlessone = multiple - 1;
	return (num + multlessone) & (-1 * multiple);
}


void PoolMemBlock_Init(struct PoolMemBlock* pMemBlock, ssize_t element_count, ssize_t element_size)
{
	pMemBlock->datablock = (void*) talloc(element_count * element_size);
	memset(pMemBlock->datablock, 0, element_count * element_size);
	ssize_t idx = 0;
	struct InplaceFreeNode* pFreeNode = (struct InplaceFreeNode*) pMemBlock->datablock;
	for(; idx < element_count - 1; ++idx)
	{
		pFreeNode->nextinplacenode = ((void*) pFreeNode + element_size);
		pFreeNode = ((void*) pFreeNode->nextinplacenode);
	}

	pFreeNode->nextinplacenode = ((void*) 0);
}

void AllocPool_Init(struct AllocPool* pAllocPool, ssize_t element_count,
		ssize_t element_size)
{
	pAllocPool->element_size = element_size;
	pAllocPool->element_count = element_count;
	pAllocPool->pool_blocks = (struct PoolMemBlock*) talloc(sizeof(struct PoolMemBlock));
	memset(pAllocPool->pool_blocks, 0, sizeof(struct PoolMemBlock));
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
				pAllocPool->element_count * 2, pAllocPool->element_size);
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
	return returnval;
}

void AllocPool_Free(struct AllocPool* pAllocPool, void* pFreeMe)
{
	//Once pFreeMe has been used (which it must be assumed to have been),
	//the nodedata and nextinplacenode have already been overwritten with data
	((struct InplaceFreeNode*)pFreeMe)->nextinplacenode = pAllocPool->headnode;
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
