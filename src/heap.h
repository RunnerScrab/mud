#ifndef HEAP_H_
#define HEAP_H_
#include <stddef.h>

struct HeapNode
{
  int key;
  void* data;
};


struct Heap
{
  struct HeapNode* array;
  size_t len, capacity;
};

void HeapNode_Copy(struct HeapNode* pSrc, struct HeapNode* pDest);
int Heap_Create(struct Heap* pHeap, int len);
int Heap_InitFromArray(struct Heap* pHeap, int* array, int len);
void Heap_Destroy(struct Heap* pHeap);

void Heap_ExtractMinimum(struct Heap* pHeap, struct HeapNode* pOut);
int Heap_MinInsert(struct Heap* pHeap, int key, void* data);

int Heap_GetSize(struct Heap* pHeap);
int Heap_IsMinHeap(struct Heap* pHeap, int idx);
void Heap_Print(struct Heap *pHeap);
void Heap_BuildMinHeap(struct Heap* pHeap);
int Heap_GetKeyAt(struct Heap* pHeap, int idx);

#endif /* HEAP_H_ */
