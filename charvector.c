#include "charvector.h"
#include "talloc.h"
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void cv_sprintf(cv_t* pcv, const char* fmt, ...)
{
	va_list arglist, cpyarglist;
	va_copy(cpyarglist, arglist);
	va_start(arglist, fmt);

	int bwritten = vsnprintf(pcv->data, pcv->capacity, fmt, arglist);
	va_end(arglist);
	va_start(cpyarglist, fmt);
	if(bwritten >= pcv->capacity)
	{
		//Our buffer wasn't large enough. Resize it!
		pcv->capacity = bwritten + 1;
		pcv->length = pcv->capacity;
		pcv->data = (el_t*) trealloc(pcv->data,
					sizeof(el_t) * pcv->capacity);
		memset(pcv->data, 0, sizeof(el_t) * pcv->capacity);
		vsnprintf(pcv->data, pcv->capacity, fmt, cpyarglist);
	}
	va_end(cpyarglist);
}

int cv_init(cv_t* cv, size_t startsize)
{
	if(startsize > 0)
	{
		cv->length = 0;
		cv->capacity = startsize << 1;
		cv->data = (el_t*) talloc(sizeof(el_t) * cv->capacity);
	}
	return (startsize > 0 && cv->data) ? 0 : -1;
}

void cv_destroy(cv_t* cv)
{
	cv->length = 0;
	cv->capacity = 0;
	tfree(cv->data);
	cv->data = 0;
}

int cv_push(cv_t* cv, el_t newel)
{
	if(cv->length >= cv->capacity)
	{
		cv->capacity = cv->capacity << 1;
		cv->data = (el_t*) trealloc(cv->data, sizeof(el_t) * cv->capacity);
	}

	if(cv->data)
	{
		cv->data[cv->length] = newel;
		++cv->length;
		return 0;
	}
	else
	{
		return -1;
	}
}

inline static void swap_elements(el_t* a, el_t* b)
{
	el_t t = *b;
	*b = *a;
	*a = t;
}

el_t cv_at(cv_t* cv, size_t idx)
{
	return cv->data[idx];
}

int cv_len(cv_t* cv)
{
	return cv->length;
}

int cv_remove(cv_t* cv, el_t targel)
{
	size_t idx = 0, len = cv->length;
	for(; idx < len; ++idx)
	{
		if(cv->data[idx] == targel)
		{
			//Since we don't care about ordering for our list of fds,
			//just zero out the fd to remove, swap it with the last element of our
			//array, then subtract 1 from the length
			cv->data[idx] = 0;
			swap_elements(&(cv->data[idx]), &(cv->data[len - 1]));
			--cv->length;

			if(cv->length <= (cv->capacity / 4))
			{
				//If we're using less than a quarter of capacity, shrink
				//the array down to twice our usage
				cv->capacity = cv->length << 1;
				cv->data = (el_t*) trealloc(cv->data, cv->capacity * sizeof(el_t));
			}
			return 0;
		}
	}

	return -1;
}
