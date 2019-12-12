#include "talloc.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

static unsigned int g_allocs = 0;
static unsigned int g_frees = 0;

struct AllocationRecord
{
	void* mem;
	char desc[32];
};

struct AllocationRecord* g_allocations = 0;
unsigned int g_allocations_len = 0;
pthread_mutex_t allocmtx;

void InitTallocSystem()
{
	pthread_mutex_init(&allocmtx, 0);
	g_allocations = (struct AllocationRecord*) malloc(sizeof(struct AllocationRecord) * 100);
	memset(g_allocations, 0, sizeof(struct AllocationRecord) * 100);
	g_allocations_len = 100;
}

void StopTallocSystem()
{
	pthread_mutex_destroy(&allocmtx);
}

void* talloc(ssize_t size, const char* desc)
{
	pthread_mutex_lock(&allocmtx);
	if(g_allocs >= g_allocations_len)
	{
		g_allocations = (struct AllocationRecord*) realloc(g_allocations, sizeof(struct AllocationRecord) * g_allocations_len * 2);
		g_allocations_len *= 2;
		memset(&(g_allocations[g_allocs]), 0, sizeof(struct AllocationRecord) * (g_allocations_len - g_allocs));
	}
	void* retval = malloc(size);
	g_allocations[g_allocs].mem = retval;
	strcpy(g_allocations[g_allocs].desc, desc);
	++g_allocs;
	pthread_mutex_unlock(&allocmtx);
	return retval;
}

void txfree(void* p)
{
	tfree_(p, "FROM ThreadTask");
}
void tfree_(void* p, const char* desc)
{
	pthread_mutex_lock(&allocmtx);
	ssize_t i = 0;
	unsigned char found = 0;
	for(; i < g_allocations_len; ++i)
	{
		if(g_allocations[i].mem == (void*) 0)
			continue;
		if(p == g_allocations[i].mem)
		{
			found = 1;
			g_allocations[i].mem = (void*) 0;
			break;
		}
	}

	if(!found)
		printf("Attempting to double free %s @ %llx!\n", desc, (long long unsigned int) p);
     ++g_frees;
     free(p);
    pthread_mutex_unlock(&allocmtx);
}

int toutstanding_allocs()
{
	unsigned int i = 0;
	for(; i < g_allocations_len; ++i)
	{
		if(g_allocations[i].mem == (void*) 0)
			continue;
		printf("%s - %llx still unfreed!\n", g_allocations[i].desc, 
		(long long unsigned int) g_allocations[i].mem);
	}
  return g_allocs - g_frees;
}

int tget_frees()
{
	return g_frees;
}

int tget_allocs()
{
	return g_allocs;
}
