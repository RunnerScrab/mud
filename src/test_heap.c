#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <check.h>

#include "heap.h"
#include "talloc.h"

int randint(int min, int max)
{
	return rand() % max + min;
}

int main(void)
{

  srand(time(NULL));
  int array[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  int array2[8] = {5, 2, 10, 1, 7, 3, 4, 9};

  struct Heap heap1;
  Heap_InitFromArray(&heap1, array, 8);
  struct Heap heap2;
  Heap_InitFromArray(&heap2, array2, 8);


  //printf("Array %s a minheap.\n", Heap_IsMinHeap(&heap1, 0) ? "is" : "is not");
  Heap_BuildMinHeap(&heap2);
  //printf("Array2 %s a minheap.\n", Heap_IsMinHeap(&heap2, 0) ? "is" : "is not");
  //Heap_Print(&heap2);

  int i = 0;
  for(; i < 1000000; ++i)
  {
	  Heap_MinInsert(&heap2, randint(1, 1000), 0);
  }
  //printf("Array2 %s a minheap.\n", Heap_IsMinHeap(&heap2, 0) ? "is" : "is not");
  //Heap_Print(&heap2);

  struct HeapNode min;
  int lastval = 0;
  for(i = 0; i < 1000000; ++i)
  {
	  Heap_ExtractMinimum(&heap2, &min);
	  assert(lastval <= min.key);
	  lastval = min.key;
  }

  Heap_ExtractMinimum(&heap2, &min);
  printf("Extracted node w/ key %d from Array2.\n", min.key);
  Heap_Print(&heap2);

  Heap_Destroy(&heap1);
  Heap_Destroy(&heap2);
  printf("%d outstanding allocations.\n", toutstanding_allocs());
  return 0;
}
