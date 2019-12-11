#ifndef POOLALLOC_H_
#define POOLALLOC_H_

#include "talloc.h"
#include <string.h>
#include <stdlib.h>


struct InplaceFreeNode
{
	struct InplaceFreeNode* nextinplacenode;
};

//Preallocated memory pool
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

void AllocPool_Init(struct AllocPool* pAllocPool, ssize_t element_count,
		ssize_t element_size);
void AllocPool_Destroy(struct AllocPool* pAllocPool);

void* AllocPool_Alloc(struct AllocPool* pAllocPool);
void AllocPool_Free(struct AllocPool* pAllocPool, void* pFreeMe);

#endif
