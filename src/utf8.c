#include <stdio.h>
#include <string.h>
#include "bitutils.h"

#ifdef __x86_64__
const char* utf8findstart(const char* str, size_t bytelen)
{
	//HACK: This routine only works on little endian machines
	unsigned long long intlen = bytelen >> 3; //bytes int-divided by the size of the int we're using; 2^3 = 8
	size_t idx = 0;
	unsigned long long* p = (unsigned long long*) str;
	unsigned long long maskedval = 0;
	for(; idx < intlen; ++idx)
	{
		maskedval = p[idx] & 0x8080808080808080;
		if(!maskedval)
		{
			continue;
		}
		size_t offset = __builtin_ctzll(maskedval) >> 3; //This must remain 3

		return &str[(idx << 3) + (offset)]; //This 3 is to multiply by 8, for # of bytes in the int we're using
	}

	//intlen will never be greater than bytelen because it is bytelen >> 2
	size_t remainder = bytelen - (intlen << 3);
	size_t charidx = (idx << 3); //multiply our integer index by the size of the int
	size_t charend = charidx + remainder;
	for(; charidx < charend; ++charidx)
	{
		if(str[charidx] & 0x80)
		{
			return &str[charidx];
		}
	}

	return 0;
}
#else
const char* utf8findstart(const char* str, size_t bytelen)
{
	//HACK: This routine only works on little endian machines
	unsigned int intlen = bytelen >> 2; //bytes int-divided by the size of the int we're using; 2^3 = 8
	size_t idx = 0;
	unsigned int* p = (unsigned int*) str;
	size_t maskedval = 0;
	for(; idx < intlen; ++idx)
	{

		maskedval = p[idx] & 0x80808080;
		if(!maskedval)
		{
			continue;
		}

		size_t offset = ntz(maskedval) >> 3; //This must remain 3

		return &str[(idx << 2) + (offset)];
	}

	//intlen will never be greater than bytelen because it is bytelen >> 2
	size_t remainder = bytelen - (intlen << 2);
	size_t charidx = (idx << 2);
	size_t charend = charidx + remainder;
	for(; charidx < charend; ++charidx)
	{
		if(str[charidx] & 0x80)
		{
			return &str[charidx];
		}
	}

	return 0;
}
#endif

size_t utf8strnlen(const char* str, size_t bytelen)
{
	size_t len = 0;
	const char* s = str;
	const char* p = utf8findstart(str, bytelen);
	size_t blen = 0;
	unsigned char utf8charwidth = 0;
	for(; p; )
	{
		utf8charwidth = leadingones(*p);
		blen += utf8charwidth + (p - s) - 1;
		len += (p - s);
		s = p + utf8charwidth;
		p = utf8findstart(s, bytelen - blen);
	}
	return len + strnlen(str + blen, bytelen - blen);
}
