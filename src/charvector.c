#include "charvector.h"
#include "talloc.h"
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define min(a, b) (a < b ? a : b)
#define max(a, b) (a > b ? a : b)

void cv_sprintf(cv_t* pcv, const char* fmt, ...)
{
	va_list arglist, cpyarglist;
	va_copy(cpyarglist, arglist);
	va_start(arglist, fmt);

	size_t bwritten = vsnprintf(pcv->data, pcv->capacity, fmt, arglist);
	va_end(arglist);
	va_start(cpyarglist, fmt);
	pcv->length = bwritten + 1;
	if(bwritten >= pcv->capacity)
	{
		//Our buffer wasn't large enough. Resize it!
		pcv->capacity = pcv->length;
		pcv->data = (el_t*) trealloc(pcv->data,
					sizeof(el_t) * pcv->capacity);
		memset(pcv->data, 0, sizeof(el_t) * pcv->capacity);
		vsnprintf(pcv->data, pcv->capacity, fmt, cpyarglist);
	}

	va_end(cpyarglist);
}

int cv_append(cv_t* cv, const el_t* data, size_t len)
{
	if((cv->length + len) > cv->capacity)
	{
		cv->capacity = max((cv->capacity << 1),(cv->capacity + len));
		cv->data = (el_t*) trealloc(cv->data,
					sizeof(el_t) * cv->capacity);
		if(!cv->data)
		{
			return -1;
		}
	}
	memcpy(&(cv->data[cv->length]), data, len * sizeof(el_t));
	cv->length = cv->length + len;
	return cv->length;
}

void cv_clear(cv_t* cv)
{
	memset(cv->data, 0, sizeof(el_t) * cv->capacity);
	cv->length = 0;
}

int cv_appendcv(cv_t* dest, cv_t* src)
{
	return cv_append(dest, src->data, src->length);
}

int cv_appendstr(cv_t* cv, const el_t* data)
{
	cv_strcat(cv, data);
	return 0;
}


int cv_init(cv_t* cv, size_t startsize)
{
	if(startsize > 0)
	{
		cv->length = 0;
		cv->capacity = startsize << 1;
		cv->data = (el_t*) talloc(sizeof(el_t) * cv->capacity);
		memset(cv->data, 0, sizeof(el_t) * cv->capacity);
	}
	return (startsize > 0 && cv->data) ? 0 : -1;
}

int cv_resize(cv_t* cv, size_t newcap)
{
	if(!cv->data || newcap != cv->capacity)
	{
		cv->data = (el_t*) trealloc(cv->data, sizeof(el_t) * newcap);
		cv->capacity = newcap;
	}
	return cv->data ? 0 : -1;
}

void cv_swap(cv_t* a, cv_t* b)
{
	cv_t temp;
	temp.data = a->data;
	temp.length = a->length;
	temp.capacity = a->capacity;

	a->data = b->data;
	a->length = b->length;
	a->capacity= b->capacity;

	b->data = temp.data;
	b->length = temp.length;
	b->capacity = temp.capacity;
}

void cv_copy(cv_t* dest, cv_t* source)
{
	dest->length = source->length;
	dest->capacity = source->capacity;
	if(dest->data)
		tfree(dest->data);
	dest->data = (el_t*) talloc(sizeof(el_t) * dest->capacity);
	memcpy(dest->data, source->data, dest->capacity);
}

void cv_strcpy(cv_t* dest, const el_t* source)
{
	dest->length = strlen(source) + 1;
	dest->capacity = dest->length;
	if(dest->data)
		tfree(dest->data);
	dest->data = (el_t*) talloc(sizeof(el_t) * dest->capacity);
	memcpy(dest->data, source, dest->capacity);
}

void cv_strncpy(cv_t* dest, const el_t* source, size_t len)
{
	dest->length = len;
	dest->capacity = len;
	if(dest->data)
		tfree(dest->data);
	dest->data = (el_t*) talloc(sizeof(el_t) * dest->capacity);
	memcpy(dest->data, source, len);
}

void cv_strncat(cv_t* dest, const el_t* source, size_t len)
{
	// To account for the null byte, the length must have 1 added to it and dest->length must have 1 subtracted
	size_t new_len = dest->length + len;
	if(new_len >= dest->capacity)
	{
		dest->capacity = max((dest->capacity << 1), new_len);
		dest->data = (el_t*) trealloc(dest->data, sizeof(el_t) * dest->capacity);
	}
	dest->length = new_len;
	strncat(dest->data, source, len);
}

void cv_strcat(cv_t* dest, const el_t* source)
{
	size_t new_len = dest->length + strlen(source);
	if(new_len >= dest->capacity)
	{
		dest->capacity = max((dest->capacity << 1), new_len);
		dest->data = (el_t*) trealloc(dest->data, sizeof(el_t) * dest->capacity);
	}
	strcat(dest->data, source);
	dest->length = new_len;
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

size_t cv_len(cv_t* cv)
{
	return cv->length;
}
