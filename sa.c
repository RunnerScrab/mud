#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int g_allocs = 0;
int g_frees = 0;

void* talloc(size_t size)
{
     ++g_allocs;
     return malloc(size);
}

void tfree(void* p)
{
     ++g_frees;
     free(p);
}

struct SockArray
{
     int* pSocks;
     size_t size, fill_pointer;
};

int SockArray_Create(struct SockArray* pArray, size_t initial_size)
{
     pArray->pSocks = (int*) talloc(sizeof(int) * initial_size, __FUNCTION__);
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

int main(void)
{
  struct SockArray sa;
  SockArray_Create(&sa, 16);
  int i = 0;
for(; i < 32; ++i)
  {
    SockArray_Push(&sa, i);
  }
SockArray_Print(&sa);
SockArray_RemoveSock(&sa, 16);
SockArray_Print(&sa);
SockArray_RemoveSock(&sa, 31);
SockArray_RemoveSock(&sa, 30);
SockArray_Print(&sa);
 SockArray_Destroy(&sa);
 printf("%d allocs, %d frees\n", g_allocs, g_frees);
 return 0;
}
