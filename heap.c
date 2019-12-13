#include "heap.h"
#include "talloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PARAMETER_K 2 //Parameter for heap treallocation if usage crosses (capacity / 2) -/+ K in either direction

#define parent(idx) ((idx - 1) >> 1)
#define left(idx) ((idx << 1) + 1)
#define right(idx) ((idx << 1) + 2)

#define min_heapify(heap, idx) min_heapify_it(heap, idx)

void HeapNode_Init(struct HeapNode* pNode, int key, void* data)
{
	pNode->key = key;
	pNode->data = data;
}

static inline void swap_heap_node(struct HeapNode* a, struct HeapNode* b)
{
	struct HeapNode t = *a;
	*a = *b;
	*b = t;
}

int Heap_GetSize(struct Heap* pHeap)
{
	return pHeap->len;
}

inline int Heap_GetKeyAt(struct Heap* pHeap, int idx)
{
	return pHeap->array[idx].key;
}

int Heap_IsMinHeap(struct Heap* pHeap, int idx)
{
	//This function is really just for debugging
	//and is not called at all by the algorithm itself
	if(idx >= pHeap->len)
	{
		return 1;
	}

	size_t rightidx = right(idx);
	size_t leftidx = left(idx);
	struct HeapNode* arr = pHeap->array;
	size_t len = pHeap->len;
	int topkey = arr[idx].key;

	return (rightidx >= len || (topkey < arr[rightidx].key && Heap_IsMinHeap(pHeap, rightidx)))
		|| (leftidx >= len || (topkey < arr[leftidx].key && Heap_IsMinHeap(pHeap, leftidx)));
}

/* static void min_heapify_rec(struct Heap* pHeap, int idx) */
/* { */
/*   int leftidx = left(idx); */
/*   int rightidx = right(idx); */
/*   int smallestidx = idx; */
/*   int arraylen = pHeap->len; */
/*   register struct HeapNode* arr = pHeap->array; */

/*   if(leftidx < arraylen && arr[idx].key > arr[leftidx].key) */
/*     { */
/*       smallestidx = leftidx; */
/*     } */
/*   if(rightidx < arraylen && arr[smallestidx].key > arr[rightidx].key) */
/*     { */
/*       smallestidx = rightidx; */
/*     } */
/*   if(idx < arraylen && smallestidx != idx) */
/*     { */
/*       swap_heap_node(&(arr[idx]), &(arr[smallestidx])); */
/*       min_heapify_rec(pHeap, smallestidx); */
/*     } */
/* } */

static inline void min_heapify_it(struct Heap* pHeap, int idx)
{
	size_t arraylen = pHeap->len;
	register struct HeapNode* arr = pHeap->array;
	char go = 0;
	do
	{
		register int leftidx = left(idx);
		register int rightidx = right(idx);
		register int smallestidx = idx;

		if(leftidx < arraylen && arr[idx].key > arr[leftidx].key)
		{
			smallestidx = leftidx;
		}
		if(rightidx < arraylen && arr[smallestidx].key > arr[rightidx].key)
		{
			smallestidx = rightidx;
		}

		go = idx < arraylen && smallestidx != idx;

		if(go)
		{
			swap_heap_node(&(arr[idx]), &(arr[smallestidx]));
			idx = smallestidx;
		}
	}
	while(go);
}


void Heap_Print(struct Heap *pHeap)
{
	int i = 0, z = pHeap->len;
	for(; i < z; ++i)
	{
		printf((i < z - 1) ? "%d," : "%d\n", Heap_GetKeyAt(pHeap, i));
	}
}

void Heap_BuildMinHeap(struct Heap* pHeap)
{
	/*
	  In a binary tree, the lowest level of leaves are always HALF of
	  ALL nodes in the tree excluding the root, because the number of
	  leaves doubles each successive level.
	*/
	int idx = ((pHeap->len)>>1) - 1;
	for(; idx >= 0; --idx)
	{
		min_heapify(pHeap, idx);
	}
}

int Heap_Create(struct Heap* pHeap, int len)
{
	pHeap->capacity = len << 1;
	pHeap->array = (struct HeapNode*) talloc(sizeof(struct HeapNode) * pHeap->capacity);
	memset(pHeap->array, 0, sizeof(struct HeapNode) * pHeap->capacity);
	pHeap->len = 0;
	return (pHeap->array) ? 0 : -1;
}

int Heap_InitFromArray(struct Heap* pHeap, int* array, int len)
{
	int i = 0;
	pHeap->capacity = (len << 1) + PARAMETER_K;
	pHeap->array = (struct HeapNode*) talloc(sizeof(struct HeapNode) * pHeap->capacity);
	memset(pHeap->array, 0, sizeof(struct HeapNode) * pHeap->capacity);
	for(; i < len; ++i)
	{
		HeapNode_Init(&(pHeap->array[i]), array[i], 0);
	}

	pHeap->len = len;

	return (pHeap->array) ? 0 : -1;
}

void Heap_DecreaseKey(struct Heap* pHeap, int idx, struct HeapNode* pNode)
{
	int i = 0;
	if(pNode->key <= pHeap->array[idx].key)
	{
		for(i = idx; i > 0 && Heap_GetKeyAt(pHeap, parent(i)) > Heap_GetKeyAt(pHeap, i);)
		{
			swap_heap_node(&(pHeap->array[i]), &(pHeap->array[parent(i)]));
			i = parent(i);
		}
	}
}

inline void HeapNode_Copy(struct HeapNode* pSrc, struct HeapNode* pDest)
{
	pDest->key = pSrc->key;
	pDest->data = pSrc->data;
}

void Heap_ExtractMinimum(struct Heap* pHeap, struct HeapNode* pOut)
{
	if(pHeap->len > 0)
	{
		struct HeapNode* pMin = &(pHeap->array[0]);

		HeapNode_Copy(pMin, pOut);

		pMin->key = 0;
		pMin->data = 0;

		swap_heap_node(&(pHeap->array[0]), &(pHeap->array[pHeap->len - 1]));


		--(pHeap->len);
		if(pHeap->len < ((pHeap->capacity >> 1) - PARAMETER_K))
		{
			pHeap->capacity = (pHeap->capacity >> 1) + PARAMETER_K;
			pHeap->array =  (struct HeapNode*) trealloc(pHeap->array,
								pHeap->capacity * sizeof(struct HeapNode));

		}

		min_heapify(pHeap, 0);
	}
	else
	{
		memset(pOut, 0, sizeof(struct HeapNode));
	}
}

const struct HeapNode* Heap_GetMinimum(struct Heap* pHeap)
{
	return &pHeap->array[0];
}

int Heap_MinInsert(struct Heap* pHeap, int key, void* data)
{
	if(pHeap->len == pHeap->capacity)
	{
		pHeap->capacity = (pHeap->capacity << 1) + PARAMETER_K;
		pHeap->array = (struct HeapNode*) trealloc(pHeap->array,
							pHeap->capacity * sizeof(struct HeapNode));
		if(!pHeap->array)
		{
			return -1;
		}
	}

	HeapNode_Init(&pHeap->array[pHeap->len], key, data);
	++(pHeap->len);
	Heap_DecreaseKey(pHeap, pHeap->len - 1, &pHeap->array[pHeap->len - 1]);
	return 0;
}

void Heap_Destroy(struct Heap* pHeap)
{
	tfree(pHeap->array);
	pHeap->len = 0;
}
