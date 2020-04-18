#include "bitutils.h"

int nlz(unsigned int x)
{
	//Goryavsky's nlz, a variation of Harley's algorithm
	static char table[64] =
			{ 32, 20, 19, 255, 255, 18, 255, 7, 10, 17, 255, 255, 14, 255, 6,
					255, 255, 9, 255, 16, 255, 255, 1, 26, 255, 13, 255, 255,
					24, 5, 255, 255, 255, 21, 255, 8, 11, 255, 15, 255, 255,
					255, 255, 2, 27, 0, 25, 255, 22, 255, 12, 255, 255, 3, 28,
					255, 23, 255, 4, 29, 255, 255, 30, 31 };

	x = x | (x >> 1);
	x = x | (x >> 2);
	x = x | (x >> 4);
	x = x | (x >> 8);
	x = x & ~(x >> 16);
	x = x * 0xfd7049ff;
	return (int) table[x >> 26];
}

int ntz(unsigned int x)
{
	//John Reiser's ntz
	static char table[37] =
			{ 32, 0, 1, 26, 2, 23, 27, 255, 3, 16, 24, 30, 28, 11, 255, 13, 4,
					7, 17, 255, 25, 22, 31, 15, 29, 10, 12, 6, 255, 21, 14, 9,
					5, 20, 8, 19, 18 };

	x = (x & -x) % 37;
	return table[x];
}

unsigned int leadingones(unsigned char byte)
{
	static const unsigned char table[256] =
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
			1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
			2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
			3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4 };
	return table[byte];
}

#ifdef __x86_64__
const char* findfirstnonspace(const char* str, size_t bytelen)
{
	//HACK: This routine only works on little endian machines
	unsigned long long intlen = bytelen >> 3;
	size_t idx = 0;
	unsigned long long* p = (unsigned long long*) str;
	unsigned long long maskedval = 0;
	for(; idx < intlen; ++idx)
	{
		maskedval = p[idx] ^ 0x2020202020202020;
		if(!maskedval)
		{
			continue;
		}
		size_t offset = __builtin_ctzll(maskedval) >> 3;
		return &str[(idx << 3) + (offset)];
	}

	size_t remainder = bytelen - (intlen << 3);
	size_t charidx = (idx << 3);
	size_t charend = charidx + remainder;
	for(; charidx < charend; ++charidx)
	{
		if(str[charidx] ^ 0x20)
		{
			return &str[charidx];
		}
	}

	return 0;
}

const char* lastnonspace(const char* str, size_t bytelen)
{
	unsigned int intlen = bytelen >> 3;
	unsigned int remainder = bytelen - (intlen << 3);

	if(remainder)
	{
		int idx = bytelen - 1;
		int margin = bytelen - remainder;
		for(; idx >= margin; --idx)
		{
			if(str[idx] ^ 0x20)
			{
				return &str[idx];
			}
		}
	}
	if(intlen)
	{
		int idx = intlen - 1;
		unsigned long long* p = (unsigned long long*) str;
		unsigned long long maskedval = 0;
		for(; idx >= 0; --idx)
		{

			maskedval = p[idx] ^ 0x2020202020202020;
			if(!maskedval)
			{
				continue;
			}
			size_t offset = __builtin_clzll(maskedval) >> 3; //This must remain 3, to convert bits to bytes
			return &str[(idx << 3) + (7 - offset)];
		}
	}

	//No non-spaces detected
	return 0;
}

#else
const char* findfirstnonspace(const char *str, size_t bytelen)
{
	//HACK: This routine only works on little endian machines
	unsigned int intlen = bytelen >> 2; //bytes int-divided by the size of the int we're using; 2^3 = 8
	size_t idx = 0;
	unsigned int *p = (unsigned int*) str;
	size_t maskedval = 0;
	for (; idx < intlen; ++idx)
	{

		maskedval = p[idx] ^ 0x20202020;
		if (!maskedval)
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
	for (; charidx < charend; ++charidx)
	{
		if (str[charidx] ^ 0x20)
		{
			return &str[charidx];
		}
	}

	return 0;
}

const char* lastnonspace(const char *str, size_t bytelen)
{
	unsigned int intlen = bytelen >> 2;
	unsigned int remainder = bytelen - (intlen << 2);

	if (remainder)
	{
		int idx = bytelen - 1;
		int margin = bytelen - remainder;
		for (; idx >= margin; --idx)
		{
			if (str[idx] ^ 0x20)
			{
				return &str[idx];
			}
		}
	}
	if (intlen)
	{
		int idx = intlen - 1;
		unsigned int *p = (unsigned int*) str;
		unsigned int maskedval = 0;
		for (; idx >= 0; --idx)
		{

			maskedval = p[idx] ^ 0x20202020;
			if (!maskedval)
			{
				continue;
			}
			size_t offset = nlz(maskedval) >> 3; //This must remain 3, to convert bits to bytes
			return &str[(idx << 2) + (3 - offset)];
		}
	}
	//No non-spaces detected
	return 0;
}

#endif
