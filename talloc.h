#ifndef TALLOC_H_
#define TALLOC_H_
#include <stddef.h>
#include <stdlib.h>

void* talloc(ssize_t size);
void* aligned_talloc(ssize_t alignment, ssize_t size);
void tfree(void* p);
int toutstanding_allocs();
int tget_frees();
int tget_allocs();

#endif
