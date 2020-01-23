#include "uuid.h"
#include <stdlib.h>
#include <stdio.h>

void UUIDToString(union UUID* uuid, cv_t* out)
{
	cv_resize(out, 37);
	snprintf(out->data,
		sizeof(char) * 37,
		"%8.8x-%4.4x-%4.4x-%2.2x%2.2x-%x%x%x%x%x%x\n",
		uuid->fields.time_low,
		uuid->fields.time_mid,
		uuid->fields.time_hi_and_version,
		uuid->fields.clock_seq_hi_and_reserved,
		uuid->fields.clock_seq_low,
		uuid->fields.nodes[0],
		uuid->fields.nodes[1],
		uuid->fields.nodes[2],
		uuid->fields.nodes[3],
		uuid->fields.nodes[4],
		uuid->fields.nodes[5]);
}

void GenerateUUID(union UUID* uuid)
{
#ifdef USEINTELINTRINSICS_
	_rdrand64_step(&uuid->qw.halfone);
	_rdrand64_step(&uuid->qw.halftwo);
#else
	unsigned long dwone = random();
	unsigned long dwtwo = random();
	unsigned long dwthree = random();
	unsigned long dwfour = random();
	uuid->qw.halfone = dwone << 32 | dwtwo;
	uuid->qw.halftwo = dwthree << 32 |  dwfour;
#endif
	uuid->fields.clock_seq_hi_and_reserved &= 63;
	uuid->fields.clock_seq_hi_and_reserved |= 128;
	uuid->fields.time_hi_and_version &= 4095;
	uuid->fields.time_hi_and_version |= 16384;
}
