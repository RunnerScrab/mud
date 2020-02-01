#include "uuid.h"
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>

void UUIDToString(union UUID* uuid, cv_t* out)
{
	cv_resize(out, 39);
	snprintf(out->data,
		sizeof(char) * 38,
		 "%8.8x-%4.4x-%4.4x-%2.2x%2.2x-%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x\n",
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
	uuid->qw.halfone = ((unsigned long long) dwone) << 32 | dwtwo;
	uuid->qw.halftwo = ((unsigned long long) dwthree) << 32 |  dwfour;
#endif
	//0000 0000
	//1000 0000
	uuid->fields.clock_seq_hi_and_reserved &= 191;
	uuid->fields.clock_seq_hi_and_reserved |= 128;
	//0000 0000 0000 0000
	//0100 0000 0000 0000
	uuid->fields.time_hi_and_version &= ((unsigned short) 20479);
	uuid->fields.time_hi_and_version |= ((unsigned short) 16384);
}
