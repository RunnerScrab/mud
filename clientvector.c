#include "clientvector.h"

int ClientVector_Init(struct ClientVector* cv, size_t startsize)
{
	if(startsize > 0)
	{
		cv->length = 0;
		cv->capacity = startsize << 1;
		cv->fds = (int*) talloc(sizeof(int) * cv->capacity);
	}
	return (startsize > 0 && cv->fds) ? 0 : -1;
}

void ClientVector_Destroy(struct ClientVector* cv)
{
	cv->length = 0;
	cv->capacity = 0;
	tfree(cv->fds);
	cv->fds = 0;
}

int ClientVector_Push(struct ClientVector* cv, int newfd)
{
	if(cv->length >= cv->capacity)
	{
		cv->capacity = cv->capacity << 1;
		cv->fds = (int*) trealloc(cv->fds, sizeof(int) * cv->capacity);
	}

	if(cv->fds)
	{
		cv->fds[cv->length] = newfd;
		++cv->length;
		return 0;
	}
	else
	{
		return -1;
	}
}

inline static void swap_ints(int* a, int* b)
{
	int t = *b;
	*b = *a;
	*a = t;
}

inline int ClientVector_At(struct ClientVector* cv, size_t idx)
{
	return cv->fds[idx];
}

int ClientVector_Remove(struct ClientVector* cv, int targfd)
{
	size_t idx = 0, len = cv->length;
	for(; idx < len; ++idx)
	{
		if(cv->fds[idx] == targfd)
		{
			//Since we don't care about ordering for our list of fds,
			//just zero out the fd to remove, swap it with the last element of our
			//array, then subtract 1 from the length
			cv->fds[idx] = 0;
			swap_ints(&(cv->fds[idx]), &(cv->fds[len - 1]));
			--cv->length;

			if(cv->length <= (cv->capacity / 4))
			{
				//If we're using less than a quarter of capacity, shrink
				//the array down to twice our usage
				cv->capacity = cv->length << 1;
				cv->fds = (int*) trealloc(cv->fds, cv->capacity * sizeof(int));
			}
			return 0;
		}
	}

	return -1;
}
