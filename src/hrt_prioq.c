#include "hrt_prioq.h"
#ifdef __cplusplus
extern "C" {
#endif

#include "talloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PARAMETER_K 2 //Parameter for heap treallocation if usage crosses (capacity / 2) -/+ K in either direction

#define parent(idx) ((idx - 1) >> 1)
#define left(idx) ((idx << 1) + 1)
#define right(idx) ((idx << 1) + 2)

#define hrt_min_heapify(heap, idx) hrt_min_heapify_it(heap, idx)

void hrt_prioqnode_init(struct hrt_prioqnode *pNode, struct timespec *key,
		void *data)
{
	pNode->key = *key;
	pNode->data = data;
}

static inline void swap_hrtprioq_node(struct hrt_prioqnode *a,
		struct hrt_prioqnode *b)
{
	struct hrt_prioqnode t = *a;
	*a = *b;
	*b = t;
}

int CmpTs(struct timespec *a, struct timespec *b)
{
	//1: a > b
	//0: a == b
	//-1: a < b
	int crit_a = (a->tv_sec > b->tv_sec) - (a->tv_sec < b->tv_sec);
	if (crit_a)
		return crit_a;

	int crit_b = (a->tv_nsec > b->tv_nsec) - (a->tv_nsec < b->tv_nsec);
	return crit_b;
}

void DiffTs(struct timespec *a, struct timespec *b, struct timespec *result)
{
	static const long ns_in_s = 1000000000;
	long ndiff = a->tv_nsec - b->tv_nsec;
	time_t tdiff = a->tv_sec - b->tv_sec;
	if (ndiff < 0)
	{
		--tdiff;
		ndiff = ns_in_s + ndiff;
	}
	result->tv_sec = tdiff;
	result->tv_nsec = ndiff;
}

size_t hrt_prioq_get_size(struct hrt_prioq *pHeap)
{
	return pHeap->len;
}

void hrt_prioq_get_key_at(struct hrt_prioq *pHeap, int idx,
		struct timespec *out)
{
	*out = pHeap->array[idx].key;
}

int hrt_prioq_isminheap(struct hrt_prioq *pHeap, size_t idx)
{
	//This function is really just for debugging
	//and is not called at all by the algorithm itself
	if (idx >= pHeap->len)
	{
		return 1;
	}

	size_t rightidx = right(idx);
	size_t leftidx = left(idx);
	struct hrt_prioqnode *arr = pHeap->array;
	size_t len = pHeap->len;
	int rightcmp = CmpTs(&arr[idx].key, &arr[rightidx].key);
	int leftcmp = CmpTs(&arr[idx].key, &arr[leftidx].key);

	return (rightidx >= len
			|| (rightcmp == -1 && hrt_prioq_isminheap(pHeap, rightidx)))
			|| (leftidx >= len
					|| (leftcmp == -1 && hrt_prioq_isminheap(pHeap, leftidx)));
}

static inline void hrt_min_heapify_it(struct hrt_prioq *pHeap, size_t idx)
{
	size_t arraylen = pHeap->len;
	register struct hrt_prioqnode *arr = pHeap->array;
	char go = 0;
	do
	{
		register size_t leftidx = left(idx);
		register size_t rightidx = right(idx);
		register size_t smallestidx = idx;

		if (leftidx < arraylen && CmpTs(&arr[idx].key, &arr[leftidx].key) == 1)
		{
			smallestidx = leftidx;
		}
		if (rightidx < arraylen
				&& CmpTs(&arr[smallestidx].key, &arr[rightidx].key) == 1)
		{
			smallestidx = rightidx;
		}

		go = idx < arraylen && smallestidx != idx;

		if (go)
		{
			swap_hrtprioq_node(&(arr[idx]), &(arr[smallestidx]));
			idx = smallestidx;
		}
	} while (go);
}

void hrt_prioq_buildminheap(struct hrt_prioq *pHeap)
{
	/*
	 In a binary tree, the lowest level of leaves are always HALF of
	 ALL nodes in the tree excluding the root, because the number of
	 leaves doubles each successive level.
	 */
	int idx = ((pHeap->len) >> 1) - 1;
	for (; idx >= 0; --idx)
	{
		hrt_min_heapify(pHeap, idx);
	}
}

int hrt_prioq_create(struct hrt_prioq *pHeap, int len)
{
	pHeap->capacity = len << 1;
	pHeap->array = (struct hrt_prioqnode*) talloc(
			sizeof(struct hrt_prioqnode) * pHeap->capacity);
	memset(pHeap->array, 0, sizeof(struct hrt_prioqnode) * pHeap->capacity);
	pHeap->len = 0;
	return (pHeap->array) ? 0 : -1;
}

void hrt_prioq_decreasekey(struct hrt_prioq *pHeap, int idx,
		struct hrt_prioqnode *pNode)
{
	int i = 0;
	if (CmpTs(&pNode->key, &pHeap->array[idx].key) <= 0)
	{
		struct timespec a, b;
		for (i = idx; i > 0;)
		{
			hrt_prioq_get_key_at(pHeap, parent(i), &a);
			hrt_prioq_get_key_at(pHeap, i, &b);
			if (CmpTs(&a, &b) == 1)
			{
				swap_hrtprioq_node(&pHeap->array[i], &pHeap->array[parent(i)]);
				i = parent(i);
			}
			else
			{
				break;
			}
		}

	}
}

void hrt_prioqnode_copy(struct hrt_prioqnode *pSrc, struct hrt_prioqnode *pDest)
{
	pDest->key = pSrc->key;
	pDest->data = pSrc->data;
}

void* hrt_prioq_extract_min(struct hrt_prioq *pHeap)
{
	if (pHeap->len > 0)
	{
		struct hrt_prioqnode *pMin = &(pHeap->array[0]);

		void *returnvalue = pMin->data;

		pMin->key.tv_nsec = 0;
		pMin->key.tv_sec = 0;
		pMin->data = 0;

		swap_hrtprioq_node(&(pHeap->array[0]), &(pHeap->array[pHeap->len - 1]));

		--(pHeap->len);
		if (pHeap->len < ((pHeap->capacity >> 1) - PARAMETER_K))
		{
			pHeap->capacity = (pHeap->capacity >> 1) + PARAMETER_K;
			pHeap->array = (struct hrt_prioqnode*) trealloc(pHeap->array,
					pHeap->capacity * sizeof(struct hrt_prioqnode));

		}

		hrt_min_heapify(pHeap, 0);
		return returnvalue;
	}
	else
	{
		return ((void*) 0);
	}

}

void hrt_prioq_extract_min_node(struct hrt_prioq *pHeap,
		struct hrt_prioqnode *pOut)
{
	if (pHeap->len > 0)
	{
		struct hrt_prioqnode *pMin = &(pHeap->array[0]);

		hrt_prioqnode_copy(pMin, pOut);

		memset(&pMin->key, 0, sizeof(struct timespec));
		pMin->data = 0;

		swap_hrtprioq_node(&(pHeap->array[0]), &(pHeap->array[pHeap->len - 1]));

		--(pHeap->len);
		if (pHeap->len < ((pHeap->capacity >> 1) - PARAMETER_K))
		{
			pHeap->capacity = (pHeap->capacity >> 1) + PARAMETER_K;
			pHeap->array = (struct hrt_prioqnode*) trealloc(pHeap->array,
					pHeap->capacity * sizeof(struct hrt_prioqnode));

		}

		hrt_min_heapify(pHeap, 0);
	}
	else
	{
		memset(pOut, 0, sizeof(struct hrt_prioqnode));
	}
}

const struct hrt_prioqnode* hrt_prioq_getminimum(struct hrt_prioq *pHeap)
{
	return &pHeap->array[0];
}

int hrt_prioq_min_insert(struct hrt_prioq *pHeap, struct timespec *key,
		void *data)
{
	if (pHeap->len == pHeap->capacity)
	{
		pHeap->capacity = (pHeap->capacity << 1) + PARAMETER_K;
		pHeap->array = (struct hrt_prioqnode*) trealloc(pHeap->array,
				pHeap->capacity * sizeof(struct hrt_prioqnode));
		if (!pHeap->array)
		{
			return -1;
		}
	}

	hrt_prioqnode_init(&pHeap->array[pHeap->len], key, data);
	++(pHeap->len);
	hrt_prioq_decreasekey(pHeap, pHeap->len - 1, &pHeap->array[pHeap->len - 1]);
	return 0;
}

void hrt_prioq_destroy(struct hrt_prioq *pHeap)
{
	tfree(pHeap->array);
	pHeap->len = 0;
}

#ifdef __cplusplus
}
#endif
