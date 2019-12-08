#include "sockarray.h"

#include "talloc.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int SockArray_Create(struct SockArray* pArray, size_t initial_size)
{
     pArray->pSocks = (int*) talloc(sizeof(int) * initial_size);
     if(!pArray->pSocks)
     {
       //ServerLog(SERVERLOG_ERROR, "Allocation failed.\n");
	  return -1;
     }
     pArray->size = initial_size;
     pArray->fill_pointer = 0;
     return 0;
}

void SockArray_Destroy(struct SockArray* pArray)
{
  tfree(pArray->pSocks);
  pArray->pSocks = 0;
}

int SockArray_Push(struct SockArray* pArray, int sock)
{
     if(pArray->fill_pointer >= pArray->size)
     {
	  pArray->pSocks = (int*) realloc(pArray->pSocks, sizeof(int) * (pArray->size * 2));
	  if(!pArray->pSocks)
	  {
	    // ServerLog(SERVERLOG_ERROR, "Allocation failed.\n");	       
	       return -1;
	  }
	  pArray->size = pArray->size * 2;
     }
     pArray->pSocks[pArray->fill_pointer] = sock;
     ++(pArray->fill_pointer);
}

int SockArray_At(struct SockArray* pArray, size_t idx)
{
     if(pArray->pSocks)
     {
	  return pArray->pSocks[idx];
     }
     else
     {
	  return -1;
     }
}

int SockArray_RemoveSock(struct SockArray* pArray, int sock)
{
     int i = 0, z = pArray->fill_pointer;
     for(; i < z; ++i)
     {
	  if(pArray->pSocks[i] == sock)
	  {
	       pArray->pSocks[i] = 0;
	       memmove(&(pArray->pSocks[i]), &(pArray->pSocks[i + 1]),
		       (pArray->fill_pointer - 1 - i) * sizeof(int));
	       --(pArray->fill_pointer);
	       //0 1 2 3 4 5
	       return 0;
	  }
     }
     return -1;
}

void SockArray_Print(struct SockArray* pArray)
{
  int i = 0, z = pArray->fill_pointer;
  for(; i < z; ++i)
    {
      printf((i < z - 1) ? "%d," : "%d\n", pArray->pSocks[i]);
    }
}
