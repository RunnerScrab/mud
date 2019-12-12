#include "talloc.h"
#include <stdlib.h>

static int g_allocs = 0;
static int g_frees = 0;

struct AllocationRecord
{
	void* mem;
	char desc[32];
};

struct AllocationRecord* g_allocations = 0;
unsigned int g_allocations_len = 0;

void* talloc(ssize_t size, const char* desc)
{
	if(!g_allocations_len)
	{
		g_allocations = malloc(sizeof(struct AllocationRecord) * 100);
		memset(g_allocations, 0, sizeof(struct AllocationRecord) * 100);
		g_allocations_len = 100;
	}

	if(g_allocs >= g_allocations_len)
	{
		g_allocations = realloc(g_allocations, sizeof(struct AllocationRecord) * g_allocations_len * 2);
		g_allocations_len *= 2;
		memset(&(g_allocations[g_allocs]), 0, sizeof(struct AllocationRecord) * (g_allocations_len - g_allocs));
	}
	void* retval = malloc(size);
	g_allocations[g_allocs].mem = retval;
	strcpy(g_allocations[g_allocs].desc, desc);
	++g_allocs;
	return retval;
}

void* aligned_talloc(ssize_t alignment, ssize_t size)
{
	++g_allocs;
	return aligned_alloc(alignment, size);
}

void txfree(void* p)
{
	tfree_(p, "FROM ThreadTask");
}
void tfree_(void* p, const char* desc)
{
	unsigned int i = 0;
	unsigned char found = 0;
	for(; i < g_allocations_len; ++i)
	{
		if(g_allocations[i].mem == 0)
			continue;
		if(p == g_allocations[i].mem)
		{
			found = 1;
			g_allocations[i].mem = 0;
			break;
		}
	}

	if(!found)
		printf("Attempting to double free %s @ %x!\n", desc, p);
     ++g_frees;
     free(p);
}

int toutstanding_allocs()
{
	unsigned int i = 0;
	for(; i < g_allocations_len; ++i)
	{
		if(g_allocations[i].mem == 0)
			continue;
		printf("%s - %x still unfreed!\n", g_allocations[i].desc, g_allocations[i].mem);
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
