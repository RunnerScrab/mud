#ifndef VECTOR_H_
#define VECTOR_H_
#include <stddef.h>

//Don't use this class anymore - it's essentially what you need to do in C to replicate
//C++ std::vector<CLASS> semantics, and it's awful. Just alloc capacity in advance with a normal
//array - this is basically all this does

struct Vector
{
     void** pStorage;
     size_t capacity, length;
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
