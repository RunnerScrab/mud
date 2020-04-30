#include "ansicolor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

struct AnsiCode
{
	const char *symbol;
	unsigned char code[16];
	unsigned int codelen;
};

//These are in alphabetical order
struct AnsiCode color_codes[] =
{
{ "bgblack", "\x1B[40m", 5 },
{ "bgblue", "\x1B[44m", 5 },
{ "bgcyan", "\x1B[46m", 5 },
{ "bggreen", "\x1B[42m", 5 },
{ "bgmagenta", "\x1B[45m", 5 },
{ "bgred", "\x1B[41m", 5 },
{ "bgwhite", "\x1B[47m", 5 },
{ "bgyellow", "\x1B[43m", 5 },

{ "black", "\x1B[30m", 5 },
{ "blink", "\x1B[05m", 5},
{ "blue", "\x1B[34m", 5 },
{ "cyan", "\x1B[36m", 5 },
{ "default", "\x1B[0m", 4 },
{ "green", "\x1B[32m", 5 },
{ "magenta", "\x1B[35m", 5 },
{ "red", "\x1B[31m", 5 },
{ "rvid", "\x1B[07m", 5},
{ "white", "\x1B[37m", 5 },
{ "yellow", "\x1B[33m", 5 }

};

static const unsigned int g_numcolcodes = sizeof(color_codes)/sizeof(struct AnsiCode);

int compcolsymbol(const void *a, const void *b)
{
	return strcmp((const char*) a,
			((const char*) ((struct AnsiCode*) b)->symbol));
}

inline int chtoi(const char ch)
{
	static const unsigned char table[] =
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, ':', ';', '<', '=', '>', '?', '@', 10, 11,
			12, 13, 14, 15 };
	return table[toupper(ch) - 48];
}

void atorgb(const char *a, size_t len, unsigned char *r, unsigned char *g,
		unsigned char *b)
{
	static const unsigned char table[] =
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 1, 2, 3, 4, 5, 6, 7, 8, 9, ':', ';', '<', '=', '>', '?', '@', 10,
			11, 12, 13, 14, 15 };
	if (strnlen(a, len) == 7 && (a[0] == '#' || a[0] == '@'))
	{
		printf("Found 24-bit ANSI color code.\n");
		*r = (table[toupper(a[1])] << 4) | table[toupper(a[2])];
		*g = (table[toupper(a[3])] << 4) | table[toupper(a[4])];
		*b = (table[toupper(a[5])] << 4) | table[toupper(a[6])];
	}
	else
	{
		*r = *g = *b = 0;
	}
}

void ANSIColorizeString(const el_t *input, size_t inputlen, cv_t *output)
{
	el_t *pInput = (el_t*) input;
	el_t *markerstart = 0;
	for (;
			*pInput
					&& (markerstart = memchr(pInput, '`',
							inputlen - (pInput - input)));)
	{
		size_t searchlen = (input + inputlen) - (markerstart + 1);
		el_t *markerend =
				markerstart ? memchr(markerstart + 1, '`', searchlen) : 0;
		el_t symbol[32] =
		{ 0 };
		if (markerstart && markerend)
		{
			strncpy(symbol, markerstart + 1, markerend - markerstart - 1);
			//strncat(output, pInput, markerstart - pInput);
			//abc`
			cv_strncat(output, pInput, markerstart - pInput);
			switch (symbol[0])
			{
			case '@':
			case '#':
			{
				//24-bit ANSI color code, foreground
				el_t twentyfourbitansi[32] =
				{ 0 };
				unsigned char r, g, b;
				atorgb(symbol, 32, &r, &g, &b);
				size_t written = snprintf(twentyfourbitansi, sizeof(el_t) * 32,
						"\x1b[%d;2;%d;%d;%dm", (symbol[0] == '#' ? 38 : 48), r,
						g, b);

				cv_appendstr(output, twentyfourbitansi, written);
			}
				break;
			default:
			{
				struct AnsiCode *found = (struct AnsiCode*) bsearch(symbol,
						color_codes, g_numcolcodes, sizeof(struct AnsiCode), compcolsymbol);
				if (found)
				{
					cv_appendstr(output, (el_t*) found->code, found->codelen);
				}
				else
				{
					cv_strncat(output, markerstart,
							markerend - markerstart + 1);
				}
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

	cv_appendstr(output, pInput, inputlen - (pInput - input) + 1);

	/*
	 int i = 0;
	 for(; i < output->length; ++i)
	 {
	 printf((i < (output->length - 1)) ? "%x," : "%x\n",
	 output->data[i]);
	 }
	 */
}
