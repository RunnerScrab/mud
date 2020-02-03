#include "rand.h"
#include <stdlib.h>

int RandGenerator_Init(struct RandGenerator* rgn)
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	rgn->seed64 =  ts.tv_sec << 32 | ts.tv_nsec;

#ifdef USEINTELINTRINSICS_
	_rdseed64_step(&rgn->seed64);
#else
	srandom(ts.tv_sec | ts.tv_nsec);
#endif
	return 0;
}

void RandGenerator_Destroy(struct RandGenerator* rgn)
{

}

void RandGenerator_Gen64(unsigned long long* p64)
{
#ifdef USEINTELINTRINSICS_
	_rdrand64_step(p64);
#else
	*p64 = random() << 32 | random();
#endif
}

#ifdef USEINTELINTRINSICS_
u_int64_t RandGenerator_PRNG64(struct RandGenerator* rgn)
{
        //PRNG algo by Vladimir Makarov
        rgn->wyhash64_x += 0x60bee2bee120fc15;
        __uint128_t tmp;
        tmp = (__uint128_t) rgn->wyhash64_x * 0xa3b195354a39b70d;
        u_int64_t m1 = (tmp >> 64) ^ tmp;
        tmp = (__uint128_t)m1 * 0x1b03738712fad5c9;
        u_int64_t m2 = (tmp >> 64) ^ tmp;
        return m2;
}

double RandGenerator_RandDouble(struct RandGenerator* rgn)
{
	static const u_int64_t max = ((u_int64_t) -1);
	return ((double) RandGenerator_PRNG64(rgn))/max;
}

#endif
int RandFIFO_Init(struct RandFIFO* fifo, size_t init_len)
{
	fifo->length = init_len;
	return 0;
}
