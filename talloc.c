#include "talloc.h"
#include <stdlib.h>

static int g_allocs = 0;
static int g_frees = 0;

void* talloc(ssize_t size)
{
     ++g_allocs;
     return malloc(size);
}

void* aligned_talloc(ssize_t alignment, ssize_t size)
{
	++g_allocs;
	return aligned_alloc(alignment, size);
}

void tfree(void* p)
{
     ++g_frees;
     free(p);
}

int toutstanding_allocs()
{
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
