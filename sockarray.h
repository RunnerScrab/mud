#ifndef SOCKARRAY_H_
#define SOCKARRAY_H_
#include <stddef.h>

struct SockArray
{
     int* pSocks;
     size_t size, fill_pointer;
};

int SockArray_Create(struct SockArray* pArray, size_t initial_size);
void SockArray_Destroy(struct SockArray* pArray);

int SockArray_Push(struct SockArray* pArray, int sock);
int SockArray_At(struct SockArray* pArray, size_t idx);
int SockArray_RemoveSock(struct SockArray* pArray, int sock);
void SockArray_Print(struct SockArray* pArray);

#endif
