#include "talloc.h"
#include "prioq/prioq.h"
#include "prioq/gc/gc.h"
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

unsigned int randint(unsigned int min, unsigned int max)
{
	return rand() % max + min;
}

int main(void)
{
	srand(time(NULL));
	_init_gc_subsystem();
	pq_t* queue = pq_init(32);

	int randarr[100] = {0};
	int i = 0;
	unsigned long long key = 0ull;
	for(; i < 10; ++i)
	{
		randarr[i%100] = i;
		key = randint(0, -1) | ((unsigned long long int) i << 32);
		insert(queue, key, &(randarr[i%100]));
	}
	for(i = 0; i < 10; ++i)
	{
		int* pVal = deletemin(queue);
		if(pVal)
			printf(i < 99 ? "%d," : "%d\n", *pVal);
	}

	pq_destroy(queue);
	_destroy_gc_subsystem();

	return 0;
}
