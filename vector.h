#ifndef VECTOR_H_
#define VECTOR_H_
#include <stddef.h>

struct Vector
{
     void** pStorage;
     size_t size, fill_pointer;
};

int Vector_Create(struct Vector* pArray, size_t initial_size);
void Vector_Destroy(struct Vector* pArray);

int Vector_Push(struct Vector* pArray, void* pVal);
void* Vector_At(struct Vector* pArray, size_t idx);
void* Vector_Find(struct Vector* pArray, void* key,
		  int (*comp)(void*, void*));
int Vector_Remove(struct Vector* pArray, void* pVal);

#endif
