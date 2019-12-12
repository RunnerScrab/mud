#ifndef TALLOC_H_
#define TALLOC_H_
#include <stddef.h>
#include <stdlib.h>

void* talloc(ssize_t size, const char* desc);

#define tfree(x) (tfree_(x, __FILE__))
void txfree(void* p);
void tfree_(void* p, const char* desc);
int toutstanding_allocs();
int tget_frees();
int tget_allocs();
void InitTallocSystem();
void StopTallocSystem();
#endif
