#ifndef TEXTUTIL_H_
#define TEXTUTIL_H_
#include <stdarg.h>
#include "charvector.h"

struct ReflowParameters
{
	//number of spaces to indent each paragraph
	unsigned char num_indent_spaces;
	//line length
	int line_width;
	unsigned char bAllowHyphenation;
};

//O(width * n), in practice sometimes perform better than the binary-search algorithm
void ReflowText(const char* input, const size_t len, cv_t* output, struct ReflowParameters* params);
void RemoveCRs(const char* in, size_t len, cv_t* output);
size_t CountNewlines(const char* in, size_t len);

#endif
