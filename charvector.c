#include "clientvector.h"

int CharVector_Init(struct CharVector* cv, size_t startsize)
{
	if(startsize > 0)
	{
		cv->length = 0;
		cv->capacity = startsize << 1;
		cv->fds = (int*) talloc(sizeof(el_t) * cv->capacity);
	}
	return (startsize > 0 && cv->fds) ? 0 : -1;
}

void CharVector_Destroy(struct CharVector* cv)
{
	cv->length = 0;
	cv->capacity = 0;
	tfree(cv->fds);
	cv->fds = 0;
}

int CharVector_Push(struct CharVector* cv, el_t newel)
{
	if(cv->length >= cv->capacity)
	{
		cv->capacity = cv->capacity << 1;
		cv->fds = (el_t*) trealloc(cv->fds, sizeof(el_t) * cv->capacity);
	}

	if(cv->fds)
	{
		cv->fds[cv->length] = newel;
		++cv->length;
		return 0;
	}
	else
	{
		return -1;
	}
}

inline static void swap_ints(el_t* a, el_t* b)
{
	el_t t = *b;
	*b = *a;
	*a = t;
}

inline el_t CharVector_At(struct CharVector* cv, size_t idx)
{
	return cv->fds[idx];
}

int CharVector_Remove(struct CharVector* cv, el_t targel)
{
	size_t idx = 0, len = cv->length;
	for(; idx < len; ++idx)
	{
		if(cv->fds[idx] == targel)
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
				cv->fds = (el_t*) trealloc(cv->fds, cv->capacity * sizeof(el_t));
			}
			return 0;
		}
	}

	return -1;
}
