#ifndef TALLOC_H_
#define TALLOC_H_
#include <stddef.h>
#include <stdlib.h>

#define talloc(size) talloc_(size, __FUNCTION__, __FILE__, __LINE__)
#define tfree(p) tfree_(p, __FUNCTION__, __FILE__, __LINE__)
#define trealloc(orig, size) trealloc_(orig, size, __FUNCTION__, __FILE__, __LINE__)

void* trealloc_(void* origp, ssize_t size, const char* func, const char* file, const int line);
void* talloc_(ssize_t size, const char* func, const char* file, const int line);
void tfree_(void* p, const char* func, const char* file, const int line);

void tfree2(void* p);
int toutstanding_allocs();
int tget_frees();
int tget_allocs();


int toutstanding_allocs();
int tget_frees();
int tget_allocs();
void tprint_summary();

#endif
