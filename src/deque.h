#ifndef DEQUE_H_
#define DEQUE_H_
#include <stddef.h>

struct deque
{
	void** data;
	size_t length, capacity;
	void (*deallocator)(void*);
};

void deque_init(struct deque* dq, size_t initial_capacity, void (*dealloc)(void*));
void* deque_at(struct deque* dq, size_t index);
size_t deque_size(struct deque* dq);
int deque_pushfront(struct deque* dq, void* nd);
int deque_pushback(struct deque* dq, void* nd);
void* deque_popfront(struct deque* dq);
void* deque_popback(struct deque* dq);
void deque_destroy(struct deque* dq);

#endif
