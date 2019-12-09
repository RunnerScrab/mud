#ifndef VECTOR_H_
#define VECTOR_H_
#include <stddef.h>

//This does NOT preserve relative order of nodes
struct Vector
{
     void** pStorage;
     size_t size, fill_pointer;
     void (*FreeNodeFn) (void*);
};

int Vector_Create(struct Vector* pArray, size_t initial_size, void (*FreeNodeFunc) (void*));
void Vector_Destroy(struct Vector* pArray);

size_t Vector_Count(struct Vector* pArray);
int Vector_Push(struct Vector* pArray, void* pVal);
void* Vector_At(struct Vector* pArray, size_t idx);
int Vector_Find(struct Vector* pArray, void* key,
		int (*comp)(void*, void*), size_t* foundidx);
int Vector_Remove(struct Vector* pArray, size_t idx);

#endif
