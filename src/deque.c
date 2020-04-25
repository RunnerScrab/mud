#include "deque.h"
#include "utils.h"
#include "talloc.h"
#include <stdlib.h>
#include <string.h>

void deque_init(struct deque* dq, size_t initial_capacity, void (*dealloc)(void*))
{
	dq->length = 0;
	dq->capacity = max(initial_capacity, 1);
	dq->data = (void**) talloc(sizeof(void*) * initial_capacity);
	memset(dq->data, 0, sizeof(void*) * initial_capacity);
	dq->deallocator = dealloc;
}

void* deque_at(struct deque* dq, size_t index)
{
	return dq->data[index];
}

size_t deque_size(struct deque* dq)
{
	return dq->length;
}

int deque_pushfront(struct deque* dq, void* nd)
{
	if(dq->length == dq->capacity)
	{
		dq->capacity = (dq->capacity << 1) + 1;
		dq->data = (void*) trealloc(dq->data, dq->capacity * sizeof(void*));
		if(!dq->data)
		{
			return -1;
		}
		memset(&dq->data[dq->length], 0, sizeof(void*) * (dq->capacity - dq->length));
	}
	if(dq->length)
	{
		memmove(&dq->data[1], &dq->data[0], sizeof(void*) * dq->length);
	}
	dq->data[0] = nd;
	return ++dq->length;
}

int deque_pushback(struct deque* dq, void* nd)
{
	if(dq->length == dq->capacity)
	{
		dq->capacity = (dq->capacity << 1) + 1;
		dq->data = (void*) trealloc(dq->data, dq->capacity * sizeof(void*));
		if(!dq->data)
		{
			return -1;
		}
		memset(&dq->data[dq->length], 0, sizeof(void*) * (dq->capacity - dq->length));
	}
	dq->data[dq->length] = nd;

	return ++dq->length;
}

void* deque_popfront(struct deque* dq)
{
	if(dq->length)
	{
		void* retval = dq->data[0];
		--dq->length;
		if(dq->length)
		{
			memmove(&dq->data[0], &dq->data[1], sizeof(void*) * (dq->length));
			dq->data[dq->length] = 0;
		}
		return retval;
	}
	return 0;
}

void* deque_popback(struct deque* dq)
{
	if(dq->length)
	{
		void* retval = dq->data[dq->length - 1];
		--dq->length;
		dq->data[dq->length] = 0;
		return retval;
	}
	return 0;
}

void deque_destroy(struct deque* dq)
{
	int i = 0, len = dq->length;
	void (*deallocate)(void*) = dq->deallocator;
	if(deallocate)
	{
		for(; i < len; ++i)
		{
			deallocate(dq->data[i]);
		}
	}
	tfree(dq->data);
}
