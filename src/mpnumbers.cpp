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

	//Comparison operators

	result = engine->RegisterObjectMethod("MPInt", "bool opEquals(const MPInt& in)",
					      asMETHODPR(MPInt, operator==, (const MPInt&) const, bool),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPInt", "bool opEquals(const double num)",
					      asMETHODPR(MPInt, operator==, (const double) const, bool),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPInt", "bool opEquals(const int num)",
					      asMETHODPR(MPInt, operator==, (const int) const, bool),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPInt", "bool opEquals(const uint32 num)",
					      asMETHODPR(MPInt, operator==, (const unsigned int) const, bool),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPInt", "int opCmp(const MPInt& in)",
					      asMETHODPR(MPInt, opCmp, (const MPInt&) const, int),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPInt", "int opCmp(const double num)",
					      asMETHODPR(MPInt, opCmp, (const double) const, int),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPInt", "int opCmp(const int num)",
					      asMETHODPR(MPInt, opCmp, (const int) const, int),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);
	result = engine->RegisterObjectMethod("MPInt", "int opCmp(const uint32 num)",
					      asMETHODPR(MPInt, opCmp, (const unsigned int) const, int),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPInt", "bool opEquals(const MPInt@ h)",
					      asMETHODPR(MPInt, isSame, (const MPInt&) const, bool),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	//Arithmetic operators
	result = engine->RegisterObjectMethod("MPInt", "MPInt@ opAdd(const MPInt& in)",
					      asMETHODPR(MPInt, operator+, (const MPInt&), MPInt*),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPInt", "MPInt@ opAdd(const uint32 num)",
					      asMETHODPR(MPInt, operator+, (const unsigned int), MPInt*),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPInt", "MPInt@ opSub(const MPInt& in)",
					      asMETHODPR(MPInt, operator+, (const MPInt&), MPInt*),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPInt", "MPInt@ opSub(const uint32 num)",
					      asMETHODPR(MPInt, operator-, (const unsigned int), MPInt*),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPInt", "MPInt@ opNeg(const MPInt& in)",
					      asMETHODPR(MPInt, operator-, (void), MPInt*),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPInt", "MPInt@ opMul(const MPInt& in)",
					      asMETHODPR(MPInt, operator*, (const MPInt&), MPInt*),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPInt", "MPInt@ opMul(const int num)",
					      asMETHODPR(MPInt, operator*, (const int), MPInt*),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPInt", "MPInt@ opMul(const uint32 num)",
					      asMETHODPR(MPInt, operator*, (const unsigned int), MPInt*),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPInt", "MPInt@ opDiv(const MPInt& in)",
					      asMETHODPR(MPInt, operator/, (const MPInt&), MPInt*),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPInt", "MPInt@ opDiv(const uint32 num)",
					      asMETHODPR(MPInt, operator/, (const unsigned int), MPInt*),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPInt", "MPInt@ opMod(const MPInt& in)",
					      asMETHODPR(MPInt, operator%, (const MPInt&), MPInt*),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPInt", "MPInt@ opPow(const int num)",
					      asMETHODPR(MPInt, pow, (const unsigned int), MPInt*),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);


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

MPInt* operator-(const unsigned int a, const MPInt& mpnum)
{
	MPInt* temporary = MPInt::Factory(0);
	mpz_ui_sub(temporary->m_value, a, mpnum.m_value);
	return temporary;
}

MPInt& MPInt::operator=(const MPInt& other)
{
	dbgprintf("MPInt copy assignment\n");
	mpz_set(m_value, other.m_value);
	AddRef();
	return *this;
}

MPInt& MPInt::operator=(const unsigned int num)
{
	dbgprintf("Unsigned int assignment\n");
	mpz_set_ui(m_value, num);
	AddRef();
	return *this;
}

MPInt& MPInt::operator=(const int num)
{
	dbgprintf("Signed int assignment: %d\n", num);
	mpz_set_si(m_value, num);
	AddRef();
	return *this;
}

MPInt& MPInt::operator=(const double num)
{
	dbgprintf("Double assignment.\n");
	mpz_set_d(m_value, num);
	AddRef();
	return *this;
}

MPInt& MPInt::operator+=(const MPInt& other)
{
	mpz_add(m_value, m_value, other.m_value);
	return *this;
}

MPInt& MPInt::operator+=(const unsigned int num)
{
	mpz_add_ui(m_value, m_value, num);
	return *this;
}

MPInt& MPInt::operator-=(const MPInt& other)
{
	mpz_sub(m_value, m_value, other.m_value);
	return *this;
}

MPInt& MPInt::operator-=(const unsigned int num)
{
	mpz_sub_ui(m_value, m_value, num);
	return *this;
}

MPInt& MPInt::operator*=(const MPInt& other)
{
	mpz_mul(m_value, m_value, other.m_value);
	return *this;
}

MPInt& MPInt::operator*=(const unsigned int num)
{
	mpz_mul_ui(m_value, m_value, num);
	return *this;
}

MPInt& MPInt::operator*=(const int num)
{
	mpz_mul_si(m_value, m_value, num);
	return *this;
}

MPInt& MPInt::operator%=(const MPInt& other)
{
	//TODO: Attempted division by zero should throw
	//an Angelscript exception somehow

	if(other == 0)
		return *this;

	mpz_tdiv_r(m_value, m_value, other.m_value);
	return *this;
}

MPInt& MPInt::operator%=(const unsigned int num)
{
	//TODO: Attempted division by zero should throw
	//an Angelscript exception somehow
	if(num == 0)
		return *this;
	mpz_tdiv_r_ui(m_value, m_value, num);
	return *this;
}

MPInt& MPInt::operator/=(const MPInt& other)
{
	mpz_tdiv_q(m_value, m_value, other.m_value);
	return *this;
}

MPInt& MPInt::operator/=(const unsigned int num)
{
	mpz_tdiv_q_ui(m_value, m_value, num);
	return *this;
}

bool MPInt::operator==(const MPInt &other) const
{
	return 0 == mpz_cmp(m_value, other.m_value);
}

bool MPInt::operator==(const double num) const
{
	return 0 == mpz_cmp_d(m_value, num);
}

bool MPInt::operator==(const int num) const
{
	return 0 == mpz_cmp_si(m_value, num);
}

bool MPInt::operator==(const unsigned int num) const
{
	return 0 == mpz_cmp_ui(m_value, num);
}

int MPInt::opCmp(const MPInt& other) const
{
	return mpz_cmp(m_value, other.m_value);
}

int MPInt::opCmp(const double num) const
{
	return mpz_cmp_d(m_value, num);
}

int MPInt::opCmp(const int num) const
{
	return mpz_cmp_si(m_value, num);
}

int MPInt::opCmp(const unsigned int num) const
{
	return mpz_cmp_ui(m_value, num);
}

bool MPInt::isSame(const MPInt& other) const
{
	return &m_value == &other.m_value;
}

bool MPInt::isNotSame(const MPInt& other) const
{
	return !isSame(other);
}

MPInt* MPInt::operator+(const MPInt& other)
{
	MPInt* temporary = MPInt::Factory(0);
	mpz_add(temporary->m_value, m_value, other.m_value);
	return temporary;
}

MPInt* MPInt::operator+(const unsigned int num)
{
	MPInt* temporary = MPInt::Factory(0);
	mpz_add_ui(temporary->m_value, m_value, num);
	return temporary;
}

MPInt* MPInt::operator-()
{
	MPInt* temporary = MPInt::Factory(0);
	mpz_neg(temporary->m_value, m_value);
	return temporary;
}

MPInt* MPInt::operator-(const MPInt& other)
{
	MPInt* temporary = MPInt::Factory(0);
	mpz_sub(temporary->m_value, m_value, other.m_value);
	return temporary;
}

MPInt* MPInt::operator-(const unsigned int num)
{
	MPInt* temporary = MPInt::Factory(0);
	mpz_sub_ui(temporary->m_value, m_value, num);
	return temporary;
}

MPInt* MPInt::operator*(const MPInt& other)
{
	MPInt* temporary = MPInt::Factory(0);
	mpz_mul(temporary->m_value, m_value, other.m_value);
	return temporary;
}

MPInt* MPInt::operator*(const unsigned int num)
{
	MPInt* temporary = MPInt::Factory(0);
	mpz_mul_ui(temporary->m_value, m_value, num);
	return temporary;
}

MPInt* MPInt::operator*(const int num)
{
	MPInt* temporary = MPInt::Factory(0);
	mpz_mul_si(temporary->m_value, m_value, num);
	return temporary;
}

MPInt* MPInt::operator/(const MPInt& other)
{
	if(other == 0)
		return 0;

	MPInt* temporary = MPInt::Factory(0);
	mpz_tdiv_q(temporary->m_value, m_value, other.m_value);
	return temporary;
}

MPInt* MPInt::operator/(const unsigned int num)
{
	if(!num)
		return 0;

	MPInt* temporary = MPInt::Factory(0);
	mpz_tdiv_q_ui(temporary->m_value, m_value, num);
	return temporary;
}

MPInt* MPInt::operator%(const MPInt& other)
{
	if(other == 0)
		return 0;

	MPInt* temporary = MPInt::Factory(0);
	mpz_tdiv_r(temporary->m_value, m_value, other.m_value);
	return temporary;
}
MPInt* MPInt::operator%(const unsigned int num)
{
	if(num == 0)
		return 0;

	MPInt* temporary = MPInt::Factory(0);
	mpz_tdiv_r_ui(temporary->m_value, m_value, num);
	return temporary;
}

MPInt* MPInt::pow(const unsigned int power)
{
	MPInt* temporary = MPInt::Factory(0);
	mpz_pow_ui(temporary->m_value, m_value, power);
	return temporary;
}

MPInt* MPInt::Abs()
{
	MPInt* temporary = MPInt::Factory(0);
	mpz_abs(temporary->m_value, m_value);
	return temporary;
}
