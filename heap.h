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

int Heap_Create(struct Heap* pHeap, size_t len);
int Heap_InitFromArray(struct Heap* pHeap, int* array, size_t len);
void Heap_Destroy(struct Heap* pHeap);

void Heap_ExtractMinimum(struct Heap* pHeap, struct HeapNode* pOut);
int Heap_MinInsert(struct Heap* pHeap, int key, void* data);

size_t Heap_GetSize(struct Heap* pHeap);
int Heap_IsMinHeap(struct Heap* pHeap, size_t idx);
void Heap_Print(struct Heap *pHeap);
void Heap_BuildMinHeap(struct Heap* pHeap);

#endif /* HEAP_H_ */
