#include "prioq.h"
#include "talloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PARAMETER_K 2 //Parameter for heap treallocation if usage crosses (capacity / 2) -/+ K in either direction

#define parent(idx) ((idx - 1) >> 1)
#define left(idx) ((idx << 1) + 1)
#define right(idx) ((idx << 1) + 2)

#define min_heapify(heap, idx) min_heapify_it(heap, idx)

void prioqnode_Init(struct prioqnode *pNode, time_t key, void *data)
{
	pNode->key = key;
	pNode->data = data;
}

static inline void swap_heap_node(struct prioqnode *a, struct prioqnode *b)
{
	struct prioqnode t = *a;
	*a = *b;
	*b = t;
}

size_t prioq_get_size(struct prioq *pHeap)
{
	return pHeap->len;
}

time_t prioq_get_key_at(struct prioq *pHeap, int idx)
{
	return pHeap->array[idx].key;
}

int prioq_IsMinHeap(struct prioq *pHeap, size_t idx)
{
	//This function is really just for debugging
	//and is not called at all by the algorithm itself
	if (idx >= pHeap->len)
	{
		return 1;
	}

	size_t rightidx = right(idx);
	size_t leftidx = left(idx);
	struct prioqnode *arr = pHeap->array;
	size_t len = pHeap->len;
	time_t topkey = arr[idx].key;

	return (rightidx >= len
			|| (topkey < arr[rightidx].key && prioq_IsMinHeap(pHeap, rightidx)))
			|| (leftidx >= len
					|| (topkey < arr[leftidx].key
							&& prioq_IsMinHeap(pHeap, leftidx)));
}

static inline void min_heapify_it(struct prioq *pHeap, size_t idx)
{
	size_t arraylen = pHeap->len;
	register struct prioqnode *arr = pHeap->array;
	char go = 0;
	do
	{
		register size_t leftidx = left(idx);
		register size_t rightidx = right(idx);
		register size_t smallestidx = idx;

		if (leftidx < arraylen && arr[idx].key > arr[leftidx].key)
		{
			smallestidx = leftidx;
		}
		if (rightidx < arraylen && arr[smallestidx].key > arr[rightidx].key)
		{
			smallestidx = rightidx;
		}

		go = idx < arraylen && smallestidx != idx;

		if (go)
		{
			swap_heap_node(&(arr[idx]), &(arr[smallestidx]));
			idx = smallestidx;
		}
	} while (go);
}

void prioq_print(struct prioq *pHeap)
{
	int i = 0, z = pHeap->len;
	for (; i < z; ++i)
	{
		printf((i < z - 1) ? "%lu," : "%lu\n", prioq_get_key_at(pHeap, i));
	}
}

void prioq_BuildMinHeap(struct prioq *pHeap)
{
	/*
	 In a binary tree, the lowest level of leaves are always HALF of
	 ALL nodes in the tree excluding the root, because the number of
	 leaves doubles each successive level.
	 */
	int idx = ((pHeap->len) >> 1) - 1;
	for (; idx >= 0; --idx)
	{
		min_heapify(pHeap, idx);
	}
}

int prioq_create(struct prioq *pHeap, int len)
{
	pHeap->capacity = len << 1;
	pHeap->array = (struct prioqnode*) talloc(
			sizeof(struct prioqnode) * pHeap->capacity);
	memset(pHeap->array, 0, sizeof(struct prioqnode) * pHeap->capacity);
	pHeap->len = 0;
	return (pHeap->array) ? 0 : -1;
}

int prioq_initfromarray(struct prioq *pHeap, int *array, int len)
{
	int i = 0;
	pHeap->capacity = (len << 1) + PARAMETER_K;
	pHeap->array = (struct prioqnode*) talloc(
			sizeof(struct prioqnode) * pHeap->capacity);
	memset(pHeap->array, 0, sizeof(struct prioqnode) * pHeap->capacity);
	for (; i < len; ++i)
	{
		prioqnode_Init(&(pHeap->array[i]), array[i], 0);
	}

	pHeap->len = len;

	return (pHeap->array) ? 0 : -1;
}

void prioq_DecreaseKey(struct prioq *pHeap, int idx, struct prioqnode *pNode)
{
	int i = 0;
	if (pNode->key <= pHeap->array[idx].key)
	{
		for (i = idx;
				i > 0
						&& prioq_get_key_at(pHeap, parent(i))
								> prioq_get_key_at(pHeap, i);)
		{
			swap_heap_node(&(pHeap->array[i]), &(pHeap->array[parent(i)]));
			i = parent(i);
		}
	}
}

void prioqnode_copy(struct prioqnode *pSrc, struct prioqnode *pDest)
{
	pDest->key = pSrc->key;
	pDest->data = pSrc->data;
}

void* prioq_extract_min(struct prioq *pHeap)
{
	if (pHeap->len > 0)
	{
		struct prioqnode *pMin = &(pHeap->array[0]);

		void *returnvalue = pMin->data;

		pMin->key = 0;
		pMin->data = 0;

		swap_heap_node(&(pHeap->array[0]), &(pHeap->array[pHeap->len - 1]));

		--(pHeap->len);
		if (pHeap->len < ((pHeap->capacity >> 1) - PARAMETER_K))
		{
			pHeap->capacity = (pHeap->capacity >> 1) + PARAMETER_K;
			pHeap->array = (struct prioqnode*) trealloc(pHeap->array,
					pHeap->capacity * sizeof(struct prioqnode));

		}

		min_heapify(pHeap, 0);
		return returnvalue;
	}
	else
	{
		return ((void*) 0);
	}

}

void prioq_extract_min_node(struct prioq *pHeap, struct prioqnode *pOut)
{
	if (pHeap->len > 0)
	{
		struct prioqnode *pMin = &(pHeap->array[0]);

		prioqnode_copy(pMin, pOut);

		pMin->key = 0;
		pMin->data = 0;

		swap_heap_node(&(pHeap->array[0]), &(pHeap->array[pHeap->len - 1]));

		--(pHeap->len);
		if (pHeap->len < ((pHeap->capacity >> 1) - PARAMETER_K))
		{
			pHeap->capacity = (pHeap->capacity >> 1) + PARAMETER_K;
			pHeap->array = (struct prioqnode*) trealloc(pHeap->array,
					pHeap->capacity * sizeof(struct prioqnode));

		}

		min_heapify(pHeap, 0);
	}
	else
	{
		memset(pOut, 0, sizeof(struct prioqnode));
	}
}

const struct prioqnode* prioq_GetMinimum(struct prioq *pHeap)
{
	return &pHeap->array[0];
}

int prioq_min_insert(struct prioq *pHeap, time_t key, void *data)
{
	if (pHeap->len == pHeap->capacity)
	{
		pHeap->capacity = (pHeap->capacity << 1) + PARAMETER_K;
		pHeap->array = (struct prioqnode*) trealloc(pHeap->array,
				pHeap->capacity * sizeof(struct prioqnode));
		if (!pHeap->array)
		{
			return -1;
		}
	}

	prioqnode_Init(&pHeap->array[pHeap->len], key, data);
	++(pHeap->len);
	prioq_DecreaseKey(pHeap, pHeap->len - 1, &pHeap->array[pHeap->len - 1]);
	return 0;
}

void prioq_destroy(struct prioq *pHeap)
{
	tfree(pHeap->array);
	pHeap->len = 0;
}
