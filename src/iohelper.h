#ifndef IOHELPER_H_
#define IOHELPER_H_
#include <stddef.h>
#include "charvector.h"


size_t read_to_cv(int fd, cv_t* cv, size_t max_read);
int write_from_cv(int fd, cv_t* cv);

#endif
