#ifndef CLIENTVECTOR_H_
#define CLIENTVECTOR_H_

#include "talloc.h"

typedef el_t unsigned char;

struct CharVector
{
	el_t* fds;
	size_t length, capacity;
};

int CharVector_Init(struct CharVector* cv, size_t startsize);
void CharVector_Destroy(struct CharVector* cv);
int CharVector_Push(struct CharVector* cv, el_t newel);
int CharVector_Remove(struct CharVector* cv, el_t targ);

#endif
