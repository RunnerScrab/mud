#include "talloc.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
static size_t g_allocs = 0;
static size_t g_frees = 0;
static size_t g_reallocs = 0;

#ifdef DEBUG
struct allocblock
{
	void* mem;
	char desc[64];
	unsigned char freed;
} *g_allocations = 0;

static size_t g_alloccount = 0;

struct allocblock* findmemintable(void* p)
{
	size_t idx = 0;
	for(; idx < g_allocs; ++idx)
	{
		if(p == g_allocations[idx].mem)
		{
			return &(g_allocations[idx]);
		}
	}
	return 0;
}
#endif

void* trealloc_(void* origp, size_t size, const char* func, const char* file, const int line)
{
	++g_reallocs;
	void *newv = realloc(origp, size);
#ifdef DEBUG
	size_t idx = 0;
	for(; idx < g_allocs; ++idx)
	{
		if(origp == g_allocations[idx].mem)
		{
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
	struct allocblock* pexistingtableentry = findmemintable(returnval);
	if(!g_allocations)
	{
		g_alloccount = 256;
		size_t blocksize = sizeof(struct allocblock) * g_alloccount;
		g_allocations = (struct allocblock*) malloc(blocksize);
		memset(g_allocations, 0, blocksize);
	}
	else if (g_alloccount <= g_allocs && !pexistingtableentry)
	{
		size_t blocksize = (g_alloccount * 2) * sizeof(struct allocblock);
		g_allocations = (struct allocblock*) trealloc(g_allocations, blocksize);
		g_alloccount *= 2;
		memset(&(g_allocations[g_allocs]), 0,
			(g_alloccount - g_allocs) * sizeof(struct allocblock));

	}


	//malloc can return a block of memory you have used before
	//if it has been freed - which is the point of freeing memory
	struct allocblock* alloc = &(g_allocations[g_allocs]);
	alloc->mem = returnval;
	sprintf(alloc->desc, pexistingtableentry ?
		"REUSED BLOCK %s-%s:%d" : "%s-%s:%d", func, file, line);


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

void tswap_memory(void** a, void** b)
{
#ifdef DEBUG
	struct allocblock *table_a = findmemintable(*a);
	struct allocblock *table_b = findmemintable(*b);
	struct allocblock table_t = *table_a;
	*table_a = *table_b;
	*table_b = table_t;
#endif
	void* t = *b;
	*b = *a;
	*a = t;
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

void talloc_subsys_release()
{
#ifdef DEBUG
	if(g_allocations)
	{
		free(g_allocations);
		g_allocations = 0;
	}
#endif
}

void tprint_summary()
{
#ifdef DEBUG
	if(g_allocations)
	{
		size_t idx = 0;
		for(; idx < g_allocs; ++idx)
		{
			printf("%p %s- freed %d times\n",
				g_allocations[idx].mem, g_allocations[idx].desc,
				g_allocations[idx].freed);
		}
		free(g_allocations);
		g_allocations = 0;
	}
	else
	{
		printf("Allocations table has already been freed.\n");
	}
	printf("%d outstanding allocations.\n", toutstanding_allocs());

#endif
}

int toutstanding_allocs()
{
	return (int) g_allocs - (int) g_frees;
}

size_t tget_frees()
{
	return g_frees;
}

size_t tget_allocs()
{
	return g_allocs;
}

size_t tget_reallocs()
{
	return g_reallocs;
}
