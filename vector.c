#include "vector.h"

#include "talloc.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int Vector_Create(struct Vector* pArray, size_t initial_size)
{
     pArray->pStorage = talloc(sizeof(void*) * initial_size);
     if(!pArray->pStorage)
     {
	  return -1;
     }
     pArray->size = initial_size;
     pArray->fill_pointer = 0;
     return 0;
}

void Vector_Destroy(struct Vector* pArray)
{
  int i = 0;
  for(; i < pArray->fill_pointer; ++i)
    {
	tfree(pArray->pStorage[i]);
    }
  tfree(pArray->pStorage);
  pArray->pStorage = 0;
}

int Vector_Push(struct Vector* pArray, void* pVal)
{
     if(pArray->fill_pointer >= pArray->size)
     {
	  pArray->pStorage = (void**) realloc(pArray->pStorage, sizeof(void*) * (pArray->size * 2));
	  if(!pArray->pStorage)
	  {
	       return -1;
	  }
	  pArray->size = pArray->size * 2;
     }
     pArray->pStorage[pArray->fill_pointer] = pVal;
     ++(pArray->fill_pointer);
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

int Vector_Remove(struct Vector* pArray, size_t idx)
{
    //Beats moving the whole chunk of memory,
    //since we're going to be O(n) for search anyway
  int z = pArray->fill_pointer;
    swap(&(pArray->pStorage[idx]), &(pArray->pStorage[z - 1]));
    tfree(pArray->pStorage[z - 1]);
    pArray->pStorage[z - 1] = 0;
    --(pArray->fill_pointer);
    return 0;
}

