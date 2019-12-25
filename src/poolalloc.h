#ifndef POOLALLOC_H_
#define POOLALLOC_H_

#include "talloc.h"
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

struct InplaceFreeNode
{
	struct InplaceFreeNode* nextinplacenode;
};

//These are all blocking data structures that should be safe to share between multiple threads.
//Allocation and freeing from the pool needs to happen in a critical section,
//but pointers to blocks in use remain valid until they are freed and should enjoy
//lock-free use

//Preallocated memory pool for particular size
struct AllocPool
{
	struct PoolMemBlock
	{
		void* datablock;
	} *pool_blocks;

	ssize_t element_size, element_count;
	struct InplaceFreeNode* headnode;
	ssize_t block_count;
	pthread_mutex_t pool_mutex;
};

struct MemoryPool
{
	//Manage alloc pools of different sizes
	struct AllocPool** alloc_pools;
	ssize_t alloc_pool_count;
	ssize_t default_init_elcount;
	pthread_mutex_t mtx;
};

void MemoryPool_Init(struct MemoryPool* mp);
void* MemoryPool_Alloc(struct MemoryPool* mp, ssize_t block_size);

//Free releases pooled memory for later reuse
void MemoryPool_Free(struct MemoryPool* mp, ssize_t block_size, void* pFreeMe);
struct AllocPool* MemoryPool_AddBlockSizePool(struct MemoryPool* mp, ssize_t block_size);

//Destroy system frees all preallocated memory from the heap via system call
void MemoryPool_Destroy(struct MemoryPool* mp);

void AllocPool_Init(struct AllocPool* pAllocPool, ssize_t element_count,
		ssize_t element_size);
void AllocPool_Destroy(struct AllocPool* pAllocPool);

void* AllocPool_Alloc(struct AllocPool* pAllocPool);
void AllocPool_Free(struct AllocPool* pAllocPool, void* pFreeMe);

#endif
