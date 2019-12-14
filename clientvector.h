#ifndef CLIENTVECTOR_H_
#define CLIENTVECTOR_H_

#include "talloc.h"


struct ClientVector
{
	int* fds;
	size_t length, capacity;
};

int ClientVector_Init(struct ClientVector* cv, size_t startsize);
void ClientVector_Destroy(struct ClientVector* cv);
int ClientVector_Push(struct ClientVector* cv, int newfd);
int ClientVector_Remove(struct ClientVector* cv, int targfd);

#endif
