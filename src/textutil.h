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

//O(n * log(n)), in practice sometimes performs better than the shortest-paths algorithm
void ReflowTextBinary(const char* input, const size_t len, cv_t* output, struct ReflowParameters* params);

//O(width * n), in practice sometimes perform better than the binary-search algorithm
void ReflowText(const char* input, const size_t len, cv_t* output, struct ReflowParameters* params);

#endif
