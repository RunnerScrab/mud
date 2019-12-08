#include "talloc.h"
#include <stdlib.h>

static int g_allocs = 0;
static int g_frees = 0;

void* talloc(size_t size)
{
     ++g_allocs;
     return malloc(size);
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
