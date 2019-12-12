#include "vector.h"

#include "talloc.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

size_t Vector_Create(struct Vector* pArray, size_t initial_size, void (*FreeNodeFunc) (void*))
{
	pArray->pStorage = (void**) talloc(sizeof(void*) * initial_size);
	if(!pArray->pStorage)
	{
		return -1;
	}
	pArray->size = initial_size;
	pArray->fill_pointer = 0;
	pArray->FreeNodeFn = FreeNodeFunc;
	return 0;
}

void Vector_Destroy(struct Vector* pArray)
{
	size_t i = 0;
	if(pArray->FreeNodeFn)
	{
		//We have been supplied a destructor for these nodes
		for(; i < pArray->fill_pointer; ++i)
		{
			pArray->FreeNodeFn(pArray->pStorage[i]);
		}
	}
	else
	{
		for(; i < pArray->fill_pointer; ++i)
		{
			tfree(pArray->pStorage[i]);
		}
	}
	tfree(pArray->pStorage);
	pArray->pStorage = 0;
}

size_t Vector_Push(struct Vector* pArray, void* pVal)
{
	if(pArray->fill_pointer >= pArray->size)
	{
		pArray->size = pArray->size * 2;
		pArray->pStorage = (void**) trealloc(pArray->pStorage, sizeof(void*) * pArray->size);
		if(!pArray->pStorage)
		{
			return -1;
		}
	}
	pArray->pStorage[pArray->fill_pointer] = pVal;
	++(pArray->fill_pointer);
	return 0;
}

size_t Vector_Count(struct Vector* pArray)
{
	return pArray->fill_pointer;
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

size_t Vector_Find(struct Vector* pArray, void* key,
		int (*comp)(void*, void*), size_t* outkey)
{
	size_t i = 0, z = pArray->fill_pointer;
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

static void swap(void** a, void** b)
{
	void* t = *b;
	*b = *a;
	*a = t;
}

size_t Vector_Remove(struct Vector* pArray, size_t idx)
{
	//Beats moving the whole chunk of memory,
	//since we're going to be O(n) for search anyway
	size_t z = pArray->fill_pointer;
	swap(&(pArray->pStorage[idx]), &(pArray->pStorage[z - 1]));

	if(pArray->FreeNodeFn)
	{
		pArray->FreeNodeFn(pArray->pStorage[z - 1]);
	}
	else
	{
		tfree(pArray->pStorage[z - 1]);
	}

	pArray->pStorage[z - 1] = 0;
	--(pArray->fill_pointer);
	return 0;
}
