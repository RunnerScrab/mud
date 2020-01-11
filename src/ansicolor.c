#include "ansicolor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

struct AnsiCode
{
	const char* symbol;
	unsigned char code[16];
	unsigned int codelen;
};

struct AnsiCode color_codes[] = {
	{"black", "\x1B[30m", 5},
	{"blue", "\x1B[34m", 5},
	{"cyan", "\x1B[36m", 5},
	{"default", "\x1B[00m", 5},
	{"green", "\x1B[32m", 5},
	{"magenta", "\x1B[35m", 5},
	{"red", "\x1B[31m", 5},
	{"white", "\x1B[37m", 5},
	{"yellow", "\x1B[33m", 5}
};

int compcolsymbol(const void* a, const void* b)
{
	return strcmp((const char*) a,
		((const char*)((struct AnsiCode*) b)->symbol));
}

inline int chtoi(const char ch)
{
	static const unsigned char table[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, ':', ';', '<', '=', '>', '?',
					      '@', 10, 11, 12, 13, 14, 15};
	return table[toupper(ch) - 48];
}

void atorgb(const char* a, unsigned char* r, unsigned char* g, unsigned char* b)
{
	static const unsigned char table[] = {
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, ':', ';', '<', '=', '>', '?',
		'@', 10, 11, 12, 13, 14, 15};
	if(strlen(a) == 7 && (a[0] == '#' || a[0] == '@'))
	{
		*r = (table[toupper(a[1])]<<4) | table[toupper(a[2])];
		*g = (table[toupper(a[3])]<<4) | table[toupper(a[4])];
		*b = (table[toupper(a[5])]<<4) | table[toupper(a[6])];
	}
	else
	{
		*r = *g = *b = 0;
	}
}

void ANSIColorizeString(const el_t* input, cv_t* output)
{
	el_t* pInput = (el_t*) input;
//	char* output = malloc(sizeof(char) * strlen(input)<<1);

	el_t* markerstart = 0;
	for(;*pInput && (markerstart = strstr(pInput, "`"));)
	{
		el_t* markerend = markerstart ? strstr(markerstart + 1, "`") : 0;
		el_t symbol[16] = {0};
		if(markerstart && markerend)
		{
			strncpy(symbol, markerstart + 1, markerend - markerstart - 1);
			//strncat(output, pInput, markerstart - pInput);
			cv_append(output, pInput, markerstart - pInput);
			switch(symbol[0])
			{
			case '@':
			case '#':
			{
				//24-bit ANSI color code, foreground
				el_t twentyfourbitansi[16] = {0};
				unsigned char r, g, b;
				atorgb(symbol, &r, &g, &b);
				snprintf(twentyfourbitansi, sizeof(el_t) * 16, "\x1b[%d;2;%d;%d;%dm",
					symbol[0] == '#'? 38 : 48,r, g, b);

				//strncat(output, twentyfourbitansi, strlen(twentyfourbitansi));
				cv_appendstr(output, twentyfourbitansi);

			}
			break;
			default:
			{
				struct AnsiCode* found = (struct AnsiCode*) bsearch(symbol, color_codes, 9,
								sizeof(struct AnsiCode), compcolsymbol);
				if(found)
					//strcat(output, found->code);
					cv_appendstr(output, (el_t*) found->code);
				else
					//strncat(output, markerstart, markerend - markerstart + 1);
					cv_append(output, markerstart, markerend - markerstart + 1);
			}
			break;
			}
			pInput = (markerend + 1);
		}
		else
		{
			break;
		}
	}
//	strcat(output, pInput);
	cv_appendstr(output, pInput);
	//printf("%s\n", output);
	//free(output);
}