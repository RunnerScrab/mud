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

void Heap_Create(struct Heap* pHeap, int len);
void Heap_InitFromArray(struct Heap* pHeap, int* array, int len);
void Heap_Destroy(struct Heap* pHeap);

void Heap_ExtractMinimum(struct Heap* pHeap, struct HeapNode* pOut);
void Heap_MinInsert(struct Heap* pHeap, int key, void* data);

int Heap_GetSize(struct Heap* pHeap);
int Heap_IsMinHeap(struct Heap* pHeap, int idx);
void Heap_Print(struct Heap *pHeap);
void Heap_BuildMinHeap(struct Heap* pHeap);

#endif /* HEAP_H_ */
