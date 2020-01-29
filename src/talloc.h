#ifndef TALLOC_H_
#define TALLOC_H_
#include <stddef.h>
#include <stdlib.h>
#ifdef __cplusplus
extern "C" {
#endif

#define talloc(size) talloc_(size, __FUNCTION__, __FILE__, __LINE__)
#define tfree(p) tfree_(p, __FUNCTION__, __FILE__, __LINE__)
#define trealloc(orig, size) trealloc_(orig, size, __FUNCTION__, __FILE__, __LINE__)

void* trealloc_(void* origp, size_t size, const char* func, const char* file, const int line);
void* talloc_(size_t size, const char* func, const char* file, const int line);
void tfree_(void* p, const char* func, const char* file, const int line);

void tswap_memory(void** a, void** b);

void tfree2(void* p);
int toutstanding_allocs();

void talloc_subsys_release();
int toutstanding_allocs();
size_t tget_frees();
size_t tget_allocs();
size_t tget_reallocs();
void tprint_summary();

#ifdef __cplusplus
}
#endif

#endif
