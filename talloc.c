#include "talloc.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
static size_t g_allocs = 0;
static size_t g_frees = 0;

#ifdef DEBUG
struct allocblock
{
	void* mem;
	char desc[64];
	unsigned char freed;
} *g_allocations = 0;
static size_t g_alloccount = 0;
#endif

void* trealloc_(void* origp, size_t size, const char* func, const char* file, const int line)
{
	void *newv = realloc(origp, size);
	#ifdef DEBUG
	size_t idx = 0;
	for(; idx < g_allocs; ++idx)
	{
		if(origp == g_allocations[idx].mem)
		{
			printf("!Moving %p to %p\n", origp, newv);
			g_allocations[idx].mem = newv;
			break;
		}
	}
	#endif
	return newv;
}

void* talloc_(size_t size, const char* func, const char* file, const int line)
{
	void* returnval = malloc(size);
	#ifdef DEBUG
	if(!g_allocations)
	{
		g_alloccount = 256;
		size_t blocksize = sizeof(struct allocblock) * g_alloccount;
		g_allocations = (struct allocblock*) malloc(blocksize);
		memset(g_allocations, 0, blocksize);
	}
	else if (g_alloccount <= g_allocs)
	{
		size_t blocksize = (g_alloccount * 2) * sizeof(struct allocblock);
		g_allocations = (struct allocblock*) trealloc(g_allocations, blocksize);
		g_alloccount *= 2;
		memset(&(g_allocations[g_allocs]), 0,
			(g_alloccount - g_allocs) * sizeof(struct allocblock));

	}
		struct allocblock* alloc = &(g_allocations[g_allocs]);
		alloc->mem = returnval;
		sprintf(alloc->desc, "%s-%s:%d", func, file, line);


	#endif
	++g_allocs;
	return returnval;

}

void tfree2(void* p)
{
	#ifdef DEBUG
	size_t idx = 0;
	for(; idx < g_allocs; ++idx)
	{
		if(p == g_allocations[idx].mem)
		{
			g_allocations[idx].freed += 1;
			break;
		}
	}
	#endif
	++g_frees;
	free(p);
}

void tfree_(void* p, const char* func, const char* file, const int line)
{
	#ifdef DEBUG
	size_t idx = 0;
	unsigned char found = 0;
	for(; idx < g_allocs; ++idx)
	{
		if(p == g_allocations[idx].mem)
		{
			g_allocations[idx].freed += 1;
			found = 1;
			break;
		}
	}

	if(!found)
	{
		printf("Couldn't find memory block at %p freed at %s-%s:%d?\n",
			p, func, file, line);
	}
	#endif
	++g_frees;
	free(p);
}

void tprint_summary()
{
	#ifdef DEBUG
	size_t idx = 0;
	for(; idx < g_allocs; ++idx)
	{
		printf("%p %s- freed %d times\n",
			g_allocations[idx].mem, g_allocations[idx].desc,
			g_allocations[idx].freed);
	}
	free(g_allocations);
	g_allocations = 0;
	#endif
}

int toutstanding_allocs()
{
	if(g_allocations)
		free(g_allocations);
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
