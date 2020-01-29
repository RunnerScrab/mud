#ifndef CLIENTVECTOR_H_
#define CLIENTVECTOR_H_

#include "talloc.h"
#include <stdarg.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef char el_t;

typedef struct
{
	el_t* data;
	size_t length, capacity;
} cv_t; // Short for "Char Vector Type"

//cv_sprintf notwithstanding, cv_t has C++ std::vector<char>-like semantics and should be
//used that way

void cv_swap(cv_t* a, cv_t* b);
void cv_cpy(cv_t* dest, cv_t* source);
int cv_init(cv_t* cv, size_t startsize);
void cv_destroy(cv_t* cv);
int cv_push(cv_t* cv, el_t newel);
int cv_resize(cv_t* cv, size_t newsize);
int cv_appendstr(cv_t* cv, el_t* data);
int cv_appendcv(cv_t* dest, cv_t* src);
int cv_append(cv_t* cv, el_t* data, size_t len);
void cv_clear(cv_t* cv);
int cv_remove(cv_t* cv, el_t targ); //Deprecated for char sequences

el_t cv_at(cv_t* cv, size_t idx);
size_t cv_len(cv_t* cv);

void cv_sprintf(cv_t* pcv, const char* fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
