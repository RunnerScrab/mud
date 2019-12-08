#ifndef TALLOC_H_
#define TALLOC_H_
#include <stddef.h>

void* talloc(size_t size);
void tfree(void* p);
int toutstanding_allocs();

#endif
