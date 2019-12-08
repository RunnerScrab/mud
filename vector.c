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
	    // ServerLog(SERVERLOG_ERROR, "Allocation failed.\n");	       
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

void* Vector_Find(struct Vector* pArray, void* key,
		int (*comp)(void*, void*))
{
  int i = 0, z = pArray->fill_pointer;
  //We can use a heap later if performance is unimpressive
  for(; i < z; ++i)
  {
    if(!comp(key, pArray->pStorage[i]))
      {
	return pArray->pStorage[i];
      }
  }
  return 0;
}

void swap(void** a, void** b)
{
  void* t = *b;
  *b = *a;
  *a = t;
}

int Vector_Remove(struct Vector* pArray, void* pVal)
{
     int i = 0, z = pArray->fill_pointer;
     for(; i < z; ++i)
     {
	  if(pArray->pStorage[i] == pVal)
	  {
	    // tfree(pVal); // i = 3, fill pointer 6, size 6
	    //move pointers from indices 4 to (4 + (6 - 1 - 3)) to idx 3 to (3 + (6 - 1 - 3))
	    swap(&(pArray->pStorage[i]), &(pArray->pStorage[z - 1]));
	    tfree(pArray->pStorage[z - 1]);
	    pArray->pStorage[z - 1] = 0;
	    --(pArray->fill_pointer);
	    //0 1 2 3 4 5|6
	    return 0;
	  }
     }
     return -1;
}

