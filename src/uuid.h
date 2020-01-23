#ifndef UUID_H_
#define UUID_H_

#ifdef USEINTELINTRINSICS_
#include <immintrin.h>
#include <stdint.h>
#endif
#include "charvector.h"

union UUID
{
	struct
	{
		u_int64_t halfone, halftwo;
	} qw;

	struct
	{
		//unsigned long long nodes :48;
		u_int32_t time_low;
		u_int16_t time_mid;
		u_int16_t time_hi_and_version;
		u_int8_t clock_seq_hi_and_reserved;
		u_int8_t clock_seq_low;
		u_int8_t nodes[6];
	} fields;
	u_int8_t bytes[16];
};

void GenerateUUID(union UUID* uuid);
void UUIDToString(union UUID* uuid, cv_t* out);

#endif
