#include "uuid.h"
#include <string>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#ifndef TESTING_
#include "as_manager.h"
#include "angelscript.h"
#endif

void ConstructUUID(UUID* uuid)
{
	new(uuid) UUID();
}

void DestructUUID(UUID* uuid)
{
	uuid->~UUID();
}

void CopyConstructUUID(UUID* newuuid, const UUID& source)
{
	new(newuuid) UUID(source);
}

#ifndef TESTING_
int RegisterUUIDClass(AngelScriptManager* manager)
{
	asIScriptEngine* pEngine = manager->engine;
	if(pEngine->RegisterObjectType("uuid", sizeof(UUID), asOBJ_VALUE) < 0)
		return -1;

	if(pEngine->RegisterObjectBehaviour("uuid", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructUUID), asCALL_CDECL_OBJFIRST) < 0)
		return -1;
	if(pEngine->RegisterObjectBehaviour("uuid", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(DestructUUID), asCALL_CDECL_OBJFIRST) < 0)
		return -1;

	if(pEngine->RegisterObjectBehaviour("uuid", asBEHAVE_CONSTRUCT,
						"void f(const uuid& in)", asFUNCTION(CopyConstructUUID), asCALL_CDECL_OBJFIRST) < 0)
		return -1;
	if(pEngine->RegisterObjectMethod("uuid", "uuid& opAssign(const uuid& in)", asMETHODPR(UUID, operator=, (const UUID&), UUID&), asCALL_THISCALL)
		< 0)
		return -1;
	if(pEngine->RegisterObjectMethod("uuid", "string ToString()",
						asMETHODPR(UUID, ToString, (void) const, std::string),
						asCALL_THISCALL) < 0)
		return -1;

	if(pEngine->RegisterObjectMethod("uuid", "bool FromString(const string& in)",
						asMETHODPR(UUID, FromString, (const std::string&), bool),
						asCALL_THISCALL) < 0)
		return -1;

	if(pEngine->RegisterObjectMethod("uuid", "void Generate()",
						asMETHODPR(UUID, Generate, (void), void), asCALL_THISCALL) < 0)
		return -1;

	return 0;
}
#endif

UUID::UUID()
{
	memset(&m_data, 0, sizeof(union UUIDunion));
}

UUID::UUID(const UUID& other)
{
	m_data = other.m_data;
}

UUID::UUID(u_int64_t half1, u_int64_t half2)
{
	m_data.qw.halfone = half1;
	m_data.qw.halftwo = half2;
}

void UUID::CopyFromByteArray(const unsigned char* in, size_t len)
{
	if(16 == len)
	{
		memcpy(&m_data.qw, in, 16);
	}
}

int UUID::CopyToByteArray(char* out, size_t len)
{
	if(len < 16)
	{
		return -1;
	}

	memcpy(out, &m_data.qw.halfone, sizeof(u_int64_t));
	memcpy(&out[sizeof(u_int64_t)], &m_data.qw.halftwo, sizeof(u_int64_t));
	return 0;
}

std::string UUID::ToString() const
{
	std::string retval;
	char temp[38] = {0};
	snprintf(temp,
		38,
		"%8.8x-%4.4x-%4.4x-%2.2x%2.2x-%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x",
		m_data.fields.time_low,
		m_data.fields.time_mid,
		m_data.fields.time_hi_and_version,
		m_data.fields.clock_seq_hi_and_reserved,
		m_data.fields.clock_seq_low,
		m_data.fields.nodes[0],
		m_data.fields.nodes[1],
		m_data.fields.nodes[2],
		m_data.fields.nodes[3],
		m_data.fields.nodes[4],
		m_data.fields.nodes[5]);
	retval = temp;
	return retval;
}

bool UUID::FromString(const std::string& str)
{
	const char* pstr = str.c_str();
	size_t len = str.size();
	if(36 != len ||
		('-' != str[8] || '-' != str[13]
			|| '-' != str[18]
			|| '-' != str[23])
		)
	{
		return false;
	}

	//8, 13, 18, 23
	char first[9] = {0};
	char second[5] = {0};
	char third[5] = {0};
	char fourth[5] = {0};
	char fifth_a[9] = {0};
	char fifth_b[5] = {0};

	strncpy(first, pstr, 8);
	strncpy(second, pstr + 9, 4);
	strncpy(third, pstr + 14, 4);
	strncpy(fourth, pstr + 19, 4);
	strncpy(fifth_a, pstr + 24, 8);
	strncpy(fifth_b, pstr + 32, 4);

	m_data.fields.time_low = strtoul(first, 0, 16);
	m_data.fields.time_mid = strtoul(second, 0, 16);
	m_data.fields.time_hi_and_version = strtoul(third, 0, 16);
	unsigned int fourthint = strtoul(fourth, 0, 16);
	m_data.fields.clock_seq_hi_and_reserved = (fourthint >> 8) & 255;
	m_data.fields.clock_seq_low = fourthint & 255;
	unsigned int fifth_a_int = htonl(strtoul(fifth_a, 0, 16));
	unsigned short fifth_b_int = htons(strtoul(fifth_b, 0, 16));

	memcpy(&m_data.fields.nodes[0], &fifth_a_int, sizeof(unsigned int));
	memcpy(&m_data.fields.nodes[sizeof(unsigned int)], &fifth_b_int, sizeof(unsigned short));

	return true;

}

void UUID::GetAsInt128(struct Int128* out)
{
	*out = m_data.qw;
}

void UUID::Generate()
{
	GenerateUUID(&m_data);
}

void UUID::GenerateUUID(union UUIDunion* uuid)
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
