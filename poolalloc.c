#include "poolalloc.h"
#include "talloc.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void MemoryPool_Init(struct MemoryPool* mp)
{
	mp->alloc_pools = 0;
	mp->alloc_pool_count = 0;
	mp->default_init_elcount = 32;
}

struct AllocPool* MemoryPool_FindAllocPool(struct MemoryPool* mp, ssize_t block_size)
{
	unsigned int idx = 0;
	for(; idx < mp->alloc_pool_count; ++idx)
	{
		if(block_size == mp->alloc_pools[idx]->element_size)
		{
			//we have found an alloc pool that manages
			//memory of block_size size, use it
			return mp->alloc_pools[idx];
		}
	}

	return ((void*) 0);
}

void* MemoryPool_Alloc(struct MemoryPool* mp, ssize_t block_size)
{
	struct AllocPool* ap = MemoryPool_FindAllocPool(mp, block_size);
	if(ap)
	{
		return AllocPool_Alloc(ap);
	}
	else
	{
		return AllocPool_Alloc(MemoryPool_AddBlockSizePool(mp, block_size));
	}
}

void MemoryPool_Free(struct MemoryPool* mp, ssize_t block_size, void* pFreeMe)
{
	struct AllocPool* ap = MemoryPool_FindAllocPool(mp, block_size);
	if(ap)
	{
		AllocPool_Free(ap, pFreeMe);
	}
}

struct AllocPool* MemoryPool_AddBlockSizePool(struct MemoryPool* mp, ssize_t block_size)
{

	if(mp->alloc_pool_count > 0)
	{
		mp->alloc_pools = trealloc(mp->alloc_pools, (1 + mp->alloc_pool_count) * sizeof(struct AllocPool*));
	}
	else
	{
		mp->alloc_pools = talloc(sizeof(struct AllocPool*));
	}

	++mp->alloc_pool_count;
	struct AllocPool** ppAllocPool = &(mp->alloc_pools[mp->alloc_pool_count - 1]);
	*ppAllocPool = talloc(sizeof(struct AllocPool));
	AllocPool_Init(*ppAllocPool, mp->default_init_elcount, block_size);
	return *ppAllocPool;
}

void MemoryPool_Destroy(struct MemoryPool* mp)
{
	ssize_t idx = 0;
	for(; idx < mp->alloc_pool_count; ++idx)
	{
		AllocPool_Destroy(mp->alloc_pools[idx]);
		tfree(mp->alloc_pools[idx]);
		mp->alloc_pools[idx] = 0;
	}
	tfree(mp->alloc_pools);
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

	pFreeNode->nextinplacenode = 0; //Not necessary b/c of the memset, but for clarity
}

void AllocPool_Init(struct AllocPool* pAllocPool, ssize_t element_count,
		ssize_t element_size)
{
	pthread_mutex_init(&(pAllocPool->pool_mutex), 0);
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
		pAllocPool->pool_blocks = (struct PoolMemBlock*) trealloc(pAllocPool->pool_blocks,
									(pAllocPool->block_count + 1) *
									sizeof(struct PoolMemBlock));
		PoolMemBlock_Init(&pAllocPool->pool_blocks[pAllocPool->block_count],
				pAllocPool->element_count * 2, pAllocPool->element_size);
		pAllocPool->element_count += pAllocPool->element_count * 2;
		++(pAllocPool->block_count);

		pAllocPool->headnode = pAllocPool->pool_blocks[pAllocPool->block_count - 1].datablock;
	}
}

void* AllocPool_Alloc(struct AllocPool* pAllocPool)
{
	pthread_mutex_lock(&(pAllocPool->pool_mutex));
	struct InplaceFreeNode* returnval = pAllocPool->headnode;
	pAllocPool->headnode = returnval->nextinplacenode;
	if(!pAllocPool->headnode)
	{
		AllocPool_AddBlock(pAllocPool);
	}
	pthread_mutex_unlock(&(pAllocPool->pool_mutex));
	return returnval;
}

void AllocPool_Free(struct AllocPool* pAllocPool, void* pFreeMe)
{
	//Once pFreeMe has been used (which it must be assumed to have been),
	//the nodedata and nextinplacenode have already been overwritten with data
	pthread_mutex_lock(&(pAllocPool->pool_mutex));
	((struct InplaceFreeNode*)pFreeMe)->nextinplacenode = pAllocPool->headnode;
	pAllocPool->headnode = pFreeMe;
	pthread_mutex_unlock(&(pAllocPool->pool_mutex));
}

void AllocPool_Destroy(struct AllocPool* pAllocPool)
{
	ssize_t idx = 0;
	for(; idx < pAllocPool->block_count; ++idx)
	{
		tfree(pAllocPool->pool_blocks[idx].datablock);
	}
	tfree(pAllocPool->pool_blocks);
	pthread_mutex_destroy(&(pAllocPool->pool_mutex));
}
