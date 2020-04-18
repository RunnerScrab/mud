#include "talloc.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
static size_t g_allocs = 0;
static size_t g_frees = 0;
static size_t g_reallocs = 0;

void* trealloc_(void *origp, size_t size, const char *func, const char *file,
		const int line)
{
	++g_reallocs;
	void *newv = realloc(origp, size);
	return newv;
}

void* talloc_(size_t size, const char *func, const char *file, const int line)
{
	void *returnval = malloc(size);
	++g_allocs;
	return returnval;

}

void tfree2(void *p)
{
	++g_frees;
	free(p);
}

void tswap_memory(void **a, void **b)
{
	void *t = *b;
	*b = *a;
	*a = t;
}

void tfree_(void *p, const char *func, const char *file, const int line)
{
	++g_frees;
	free(p);
}

void tprint_summary()
{
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
