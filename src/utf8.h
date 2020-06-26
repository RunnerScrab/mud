#ifndef UTF8_H_
#define UTF8_H_

const char* utf8findstart(const char* str, size_t bytelen);
size_t utf8strnlen(const char* str, size_t bytelen);
int nlz(unsigned int x);
int ntz(unsigned int x);

#endif
