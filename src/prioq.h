#ifndef HEAP_H_
#define HEAP_H_
#include <stddef.h>
#include <time.h>

struct prioqnode
{
  time_t key;
  void* data;
};

struct prioq
{
  struct prioqnode* array;
  size_t len, capacity;
};

void prioqnode_copy(struct prioqnode* pSrc, struct prioqnode* pDest);
int prioq_create(struct prioq* pHeap, int len);
int prioq_initfromarray(struct prioq* pHeap, int* array, int len);
void prioq_destroy(struct prioq* pHeap);

void* prioq_extract_min(struct prioq* pHeap);
void prioq_extract_min_node(struct prioq* pHeap, struct prioqnode* pOut);

//Perhaps not the most apt name, MinInsert is the main function used to
//enqueue new nodes
int prioq_min_insert(struct prioq* pHeap, time_t key, void* data);

size_t prioq_get_size(struct prioq* pHeap);

void prioq_print(struct prioq *pHeap);

time_t prioq_get_key_at(struct prioq* pHeap, int idx);

#endif /* HEAP_H_ */
