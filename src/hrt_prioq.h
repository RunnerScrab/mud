#ifndef HRT_PRIOQ_H_
#define HRT_PRIOQ_H_
#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif


struct hrt_prioqnode
{
	struct timespec key;
	void *data;
};

struct hrt_prioq
{
	struct hrt_prioqnode *array;
	size_t len, capacity;
};

void hrt_prioqnode_init(struct hrt_prioqnode *pNode, struct timespec *key,
		void *data);
int CmpTs(struct timespec *a, struct timespec *b);
void DiffTs(struct timespec *a, struct timespec *b, struct timespec *result);

size_t hrt_prioq_get_size(struct hrt_prioq *pHeap);
void hrt_prioq_get_key_at(struct hrt_prioq *pHeap, int idx,
		struct timespec *out);
int hrt_prioq_create(struct hrt_prioq *pHeap, int len);
void hrt_prioqnode_copy(struct hrt_prioqnode *pSrc,
		struct hrt_prioqnode *pDest);
void* hrt_prioq_extract_min(struct hrt_prioq *pHeap);
void hrt_prioq_extract_min_node(struct hrt_prioq *pHeap,
		struct hrt_prioqnode *pOut);
const struct hrt_prioqnode* hrt_prioq_getminimum(struct hrt_prioq *pHeap);
int hrt_prioq_min_insert(struct hrt_prioq *pHeap, struct timespec *key,
		void *data);
void hrt_prioq_destroy(struct hrt_prioq *pHeap);
int hrt_prioq_isminheap(struct hrt_prioq *pHeap, size_t idx);
#ifdef __cplusplus
}
#endif

#endif
