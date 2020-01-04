#include "vector.h"
#include "talloc.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int Vector_Create(struct Vector* pArray, size_t initial_size, void (*FreeNodeFunc) (void*))
{
	pArray->pStorage = (void**) talloc(sizeof(void*) * initial_size);

	if(!pArray->pStorage)
	{
		return -1;
	}

	pArray->capacity = initial_size;
	pArray->length = 0;
	pArray->FreeNodeFn = FreeNodeFunc;
	return 0;
}

void Vector_Destroy(struct Vector* pArray)
{
	size_t i = 0;
	if(pArray->FreeNodeFn)
	{
		//We have been supplied a destructor for these nodes
		for(; i < pArray->length; ++i)
		{
			pArray->FreeNodeFn(pArray->pStorage[i]);
		}
	}
	else
	{
		for(; i < pArray->length; ++i)
		{
			tfree(pArray->pStorage[i]);
		}
	}
	tfree(pArray->pStorage);
	pArray->pStorage = 0;
}

int Vector_Push(struct Vector* pArray, void* pVal)
{
	if(pArray->length >= pArray->capacity)
	{
		pArray->capacity = pArray->capacity << 1;
		pArray->pStorage = (void**) trealloc(pArray->pStorage, sizeof(void*) * pArray->capacity);
		if(!pArray->pStorage)
		{
			return -1;
		}
	}
	pArray->pStorage[pArray->length] = pVal;
	++(pArray->length);
	return 0;
}

size_t Vector_Count(struct Vector* pArray)
{
	return pArray->length;
}

void* Vector_At(struct Vector* pArray, size_t idx)
{
	if(pArray->pStorage)
	{
		return pArray->pStorage[idx];
	}
	else
	{
		return (void*) 0;
	}
}

int Vector_Find(struct Vector* pArray, void* key,
		int (*comp)(void*, void*), size_t* outkey)
{
	size_t i = 0, z = pArray->length;
	//We can use a heap later if performance is unimpressive
	for(; i < z; ++i)
	{
		if(!comp(key, pArray->pStorage[i]))
		{
			*outkey = i;
			return 0;
		}
	}
	return -1;
}

int Vector_Remove(struct Vector* pArray, size_t idx)
{
	if(pArray->FreeNodeFn)
	{
		pArray->FreeNodeFn(pArray->pStorage[idx]);
	}
	else
	{
		tfree(pArray->pStorage[idx]);
	}

	memmove(&(pArray->pStorage[idx]), &(pArray->pStorage[idx + 1]),
		(pArray->length - idx - 1) * sizeof(void*));

	if(pArray->length <= (pArray->capacity >> 2))
	{
		pArray->capacity = pArray->length << 1;
		pArray->pStorage = (void**) trealloc(pArray->pStorage, pArray->capacity * sizeof(void*));
	}

	--(pArray->length);
	return 0;
}
