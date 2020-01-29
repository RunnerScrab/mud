#ifndef RAND_H_
#define RAND_H_

#include <time.h>

#ifdef USEINTELINTRINSICS_
#include <immintrin.h>
#include <stdint.h>
#endif

#include <math.h>
#include <stdarg.h>

struct RandGenerator
{
	unsigned long long seed64;
	unsigned long long wyhash64_x;
};

struct RandFIFO
{
	unsigned long long* fifo;
	size_t length;
};

int RandFIFO_Init(struct RandFIFO* fifo, size_t init_len);
int RandGenerator_Init(struct RandGenerator* rgn);
void RandGenerator_Destroy(struct RandGenerator* rgn);
void RandGenerator_Gen64(unsigned long long* p64);

#endif
