#ifndef CLIENTVECTOR_H_
#define CLIENTVECTOR_H_

#include "talloc.h"
#include <stdarg.h>

typedef char el_t;

typedef struct
{
	el_t* data;
	size_t length, capacity;
} cv_t; // Short for "Char Vector Type"

void cv_cpy(cv_t* dest, cv_t* source);
int cv_init(cv_t* cv, size_t startsize);
void cv_destroy(cv_t* cv);
int cv_push(cv_t* cv, el_t newel);
int cv_remove(cv_t* cv, el_t targ); //Deprecated for char sequences

el_t cv_at(cv_t* cv, size_t idx);
int cv_len(cv_t* cv);

void cv_sprintf(cv_t* pcv, const char* fmt, ...);

#endif
