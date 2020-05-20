#include "mpnumbers.h"
#include "angelscript.h"
#include "utils.h"
#include <vector>

MemoryPoolAllocator* MPInt::m_static_mempool;

int MultiPrecisionLibrary_Init()
{
	MPInt::m_static_mempool = new MemoryPoolAllocator(sizeof(MPInt));
	return 0;
}

int MultiPrecisionLibrary_Teardown()
{
	delete MPInt::m_static_mempool;
	MPInt::m_static_mempool = 0;
	return 0;
}

int RegisterMPIntClass(asIScriptEngine* engine)
{
	int result = 0;
	result = engine->RegisterObjectType("MPInt", 0, asOBJ_REF);
	RETURNFAIL_IF(result < 0);

	//Reference and memory management
	result = engine->RegisterObjectBehaviour("MPInt", asBEHAVE_ADDREF, "void f()",
						asMETHOD(MPInt, AddRef), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);
	result = engine->RegisterObjectBehaviour("MPInt", asBEHAVE_RELEASE, "void f()",
						asMETHOD(MPInt, Release), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);
	result = engine->RegisterObjectBehaviour("MPInt", asBEHAVE_GET_WEAKREF_FLAG,
						"int& f()", asMETHOD(MPInt, GetWeakRefFlag),
						asCALL_THISCALL);
	result = engine->RegisterObjectBehaviour("MPInt", asBEHAVE_FACTORY,
						"MPInt@ f(int32 num = 0)", asFUNCTION(MPInt::Factory),
						asCALL_CDECL);
	RETURNFAIL_IF(result < 0);

	//Assignment operators

	result = engine->RegisterObjectMethod("MPInt", "MPInt@ opAssign(const MPInt &in)",
					asMETHODPR(MPInt, operator=, (const MPInt&), MPInt&),
					asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPInt", "MPInt@ opAssign(const uint32 n)",
					asMETHODPR(MPInt, operator=, (const unsigned int), MPInt&),
					asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);


	result = engine->RegisterObjectMethod("MPInt", "MPInt@ opAssign(const int n)",
					asMETHODPR(MPInt, operator=, (const int), MPInt&),
					asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPInt", "MPInt@ opAssign(const double n)",
					asMETHODPR(MPInt, operator=, (const double), MPInt&),
					asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	/*
	result = engine->RegisterObjectMethod("MPInt", "MPInt@ opAssign(const double n)",
					asMETHODPR(MPInt, operator=, (const double), MPInt&),
					asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);
	*/

	//Comparison operators

	//Conversion operations
	result = engine->RegisterObjectMethod("MPInt", "const string toString()",
					asMETHOD(MPInt, toString), asCALL_THISCALL);
	return result;
}

int RegisterMPNumberClasses(asIScriptEngine* engine)
{
	int result = 0;
	result = RegisterMPIntClass(engine);
	return result;
}

const std::string MPInt::toString()
{
	size_t minsize = mpz_sizeinbase(m_value, 10) + 2;
	std::string retval;
	retval.resize(minsize);
	mpz_get_str(&retval[0], 10, m_value);
	retval.resize(retval.find('\0'));
	dbgprintf("Converting mpint to %s\n", retval.c_str());
	return retval;
}
