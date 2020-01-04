#ifndef HEAP_H_
#define HEAP_H_
#include <stddef.h>
#include <time.h>

struct HeapNode
{
  time_t key;
  void* data;
};

//This is called "Heap", and it is, but it's only used as a priority queue
//in this program
struct Heap
{
  struct HeapNode* array;
  size_t len, capacity;
};

void HeapNode_Copy(struct HeapNode* pSrc, struct HeapNode* pDest);
int Heap_Create(struct Heap* pHeap, int len);
int Heap_InitFromArray(struct Heap* pHeap, int* array, int len);
void Heap_Destroy(struct Heap* pHeap);

void* Heap_ExtractMinimum(struct Heap* pHeap);
void Heap_ExtractMinimumNode(struct Heap* pHeap, struct HeapNode* pOut);

//Perhaps not the most apt name, MinInsert is the main function used to
//enqueue new nodes
int Heap_MinInsert(struct Heap* pHeap, time_t key, void* data);

size_t Heap_GetSize(struct Heap* pHeap);

void Heap_Print(struct Heap *pHeap);

time_t Heap_GetKeyAt(struct Heap* pHeap, int idx);

#endif /* HEAP_H_ */
