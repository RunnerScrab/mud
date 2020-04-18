#ifndef BITUTILS_H_
#define BITUTILS_H_
#include <stddef.h>

int nlz(unsigned int x); //Number of Leading Zeros
int ntz(unsigned int x); //Number of Trailing Zeros
unsigned int leadingones(unsigned char byte);
const char* findfirstnonspace(const char *str, size_t bytelen);
const char* lastnonspace(const char *str, size_t bytelen);

#endif
