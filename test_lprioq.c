#include "talloc.h"
#include "prioq/prioq.h"
#include "prioq/gc/gc.h"
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

int randint(int min, int max)
{
	return rand() % max + min;
}

int main(void)
{
	srand(time(NULL));
	_init_gc_subsystem();
	pq_t* queue = pq_init(0);

	int randarr[100] = {0};
	int i = 0;
	for(; i < 100; ++i)
	{
		randarr[i] = i;
		insert(queue, randint(1, 10), &(randarr[i]));
	}
	for(i = 0; i < 100; ++i)
	{
		int* pVal = deletemin(queue);
		if(pVal)
			printf(i < 99 ? "%d," : "%d\n", *pVal);
	}

	pq_destroy(queue);
	return 0;
}
