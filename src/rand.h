#ifndef RAND_H_
#define RAND_H_

#include <time.h>

#ifdef USEINTELINTRINSICS_
#include <immintrin.h>
#include <stdint.h>
#endif

#include <math.h>

struct RandGenerator
{
	unsigned long long seed64;
	u_int64_t wyhash64_x;
};

struct RandFIFO
{
	u_int64_t* fifo;
	size_t length;
};

int RandFIFO_Init(struct RandFIFO* fifo, size_t init_len);
int RandGenerator_Init(struct RandGenerator* rgn);
void RandGenerator_Destroy(struct RandGenerator* rgn);
inline void RandGenerator_Gen64(unsigned long long* p64);

#endif
