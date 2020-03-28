#ifndef UUID_H_
#define UUID_H_

#ifdef USEINTELINTRINSICS_
#include <immintrin.h>
#endif

#include <string>
#include <stdint.h>
#include "charvector.h"

#ifndef TESTING_
#include "as_manager.h"
#endif

struct Int128
{
	u_int64_t halfone, halftwo;
};

union UUIDunion
{
	struct Int128 qw;

	struct
	{
		u_int32_t time_low;
		u_int16_t time_mid;
		u_int16_t time_hi_and_version;
		u_int8_t clock_seq_hi_and_reserved;
		u_int8_t clock_seq_low;
		u_int8_t nodes[6];
	} fields;
	u_int8_t bytes[16];
};

class UUID
{
	union UUIDunion m_data;
public:
	UUID(); //No arguments generates UUID
	UUID(const UUID& other); //Copy constructor
	UUID(u_int64_t half1, u_int64_t half2);
	void CopyFromByteArray(const unsigned char* in, size_t len);
	int CopyToByteArray(char* out, size_t len);
	void GetAsInt128(struct Int128* out);
	std::string ToString() const;
	bool FromString(const std::string& str);

	const char* GetData() const
	{
		return reinterpret_cast<const char*>(m_data.bytes);
	}

	size_t GetDataSize() const
	{
		return 16;
	}

	void Generate();
	static void GenerateUUID(union UUIDunion* uuid);
};

#ifndef TESTING_
int RegisterUUIDClass(AngelScriptManager* manager);
#endif


#endif
