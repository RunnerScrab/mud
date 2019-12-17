#ifndef IOHELPER_H_
#define IOHELPER_H_
#include <stddef.h>
#include "charvector.h"

size_t read_to_cv(int fd, cv_t* cv, size_t startidx, size_t max_read);
int write_from_cv(int fd, cv_t* cv);
int write_full(int fd, char* msg, size_t len);

#endif
