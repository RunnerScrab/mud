#include "mpfloat.h"
#include "angelscript.h"
#include <vector>

MemoryPoolAllocator* MPFloat::m_static_mempool;

int RegisterMPFloatClass(asIScriptEngine* engine)
{
	int result = 0;

	result = engine->RegisterObjectBehaviour("MPFloat", asBEHAVE_CONSTRUCT, "void f()",
						 asFUNCTION(MPFloat::ASConstructor), asCALL_CDECL_OBJLAST);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectBehaviour("MPFloat", asBEHAVE_DESTRUCT, "void f()",
						 asFUNCTION(MPFloat::ASDestructor), asCALL_CDECL_OBJLAST);
	RETURNFAIL_IF(result < 0);
	//Assignment operators

	result = engine->RegisterObjectMethod("MPFloat", "MPFloat& opAssign(const MPFloat &in)",
					      asMETHODPR(MPFloat, operator=, (const MPFloat&), MPFloat&),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPFloat", "MPFloat& opAssign(const uint32 n)",
					      asMETHODPR(MPFloat, operator=, (const unsigned int), MPFloat&),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);


	result = engine->RegisterObjectMethod("MPFloat", "MPFloat& opAssign(const int n)",
					      asMETHODPR(MPFloat, operator=, (const int), MPFloat&),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPFloat", "MPFloat& opAssign(const double n)",
					      asMETHODPR(MPFloat, operator=, (const double), MPFloat&),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPFloat", "MPFloat& opAssign(const string& in)",
					      asMETHODPR(MPFloat, operator=, (const std::string&), MPFloat&),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);


	//Comparison operators

	result = engine->RegisterObjectMethod("MPFloat", "bool opEquals(const MPFloat& in)",
					      asMETHODPR(MPFloat, operator==, (const MPFloat&) const, bool),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPFloat", "bool opEquals(const double num)",
					      asMETHODPR(MPFloat, operator==, (const double) const, bool),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPFloat", "bool opEquals(const int num)",
					      asMETHODPR(MPFloat, operator==, (const int) const, bool),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPFloat", "bool opEquals(const uint32 num)",
					      asMETHODPR(MPFloat, operator==, (const unsigned int) const, bool),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPFloat", "int opCmp(const MPFloat& in)",
					      asMETHODPR(MPFloat, opCmp, (const MPFloat&) const, int),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPFloat", "int opCmp(const double num)",
					      asMETHODPR(MPFloat, opCmp, (const double) const, int),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPFloat", "int opCmp(const int num)",
					      asMETHODPR(MPFloat, opCmp, (const int) const, int),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);
	result = engine->RegisterObjectMethod("MPFloat", "int opCmp(const uint32 num)",
					      asMETHODPR(MPFloat, opCmp, (const unsigned int) const, int),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	//Arithmetic-assignment operators

	result = engine->RegisterObjectMethod("MPFloat", "MPFloat& opAddAssign(const MPFloat& in)",
					      asMETHODPR(MPFloat, operator+=, (const MPFloat&), MPFloat&),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPFloat", "MPFloat& opAddAssign(const uint32 num)",
					      asMETHODPR(MPFloat, operator+=, (const unsigned int), MPFloat&),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPFloat", "MPFloat& opSubAssign(const MPFloat& in)",
					      asMETHODPR(MPFloat, operator-=, (const MPFloat&), MPFloat&),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPFloat", "MPFloat& opSubAssign(const uint32 num)",
					      asMETHODPR(MPFloat, operator-=, (const unsigned int), MPFloat&),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPFloat", "MPFloat& opMulAssign(const MPFloat& in)",
					      asMETHODPR(MPFloat, operator*=, (const MPFloat&), MPFloat&),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPFloat", "MPFloat& opMulAssign(const uint32 num)",
					      asMETHODPR(MPFloat, operator*=, (const unsigned int), MPFloat&),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);
	result = engine->RegisterObjectMethod("MPFloat", "MPFloat& opDivAssign(const MPFloat& in)",
					      asMETHODPR(MPFloat, operator/=, (const MPFloat&), MPFloat&),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPFloat", "MPFloat& opDivAssign(const uint32 num)",
					      asMETHODPR(MPFloat, operator/=, (const unsigned int), MPFloat&),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPFloat", "MPFloat& opPowAssign(const uint32 num)",
					      asMETHODPR(MPFloat, powassign, (const unsigned int), MPFloat&),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	//Arithmetic operators
	result = engine->RegisterObjectMethod("MPFloat", "MPFloat& opAdd(const MPFloat& in)",
					      asMETHODPR(MPFloat, operator+, (const MPFloat&), MPFloat*),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPFloat", "MPFloat& opAdd(const uint32 num)",
					      asMETHODPR(MPFloat, operator+, (const unsigned int), MPFloat*),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPFloat", "MPFloat& opSub(const MPFloat& in)",
					      asMETHODPR(MPFloat, operator+, (const MPFloat&), MPFloat*),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPFloat", "MPFloat& opSub(const uint32 num)",
					      asMETHODPR(MPFloat, operator-, (const unsigned int), MPFloat*),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPFloat", "MPFloat& opNeg(const MPFloat& in)",
					      asMETHODPR(MPFloat, operator-, (void), MPFloat*),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPFloat", "MPFloat& opMul(const MPFloat& in)",
					      asMETHODPR(MPFloat, operator*, (const MPFloat&), MPFloat*),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPFloat", "MPFloat& opMul(const uint32 num)",
					      asMETHODPR(MPFloat, operator*, (const unsigned int), MPFloat*),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPFloat", "MPFloat& opDiv(const MPFloat& in)",
					      asMETHODPR(MPFloat, operator/, (const MPFloat&), MPFloat*),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPFloat", "MPFloat& opDiv(const uint32 num)",
					      asMETHODPR(MPFloat, operator/, (const unsigned int), MPFloat*),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPFloat", "MPFloat& opPow(const int num)",
					      asMETHODPR(MPFloat, pow, (const unsigned int), MPFloat*),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	//Conversion operations
	result = engine->RegisterObjectMethod("MPFloat", "const string toString(int digits = 3)",
					      asMETHOD(MPFloat, toString), asCALL_THISCALL);
	return result;
}

MPFloat::MPFloat(const MPInt& other)
{
	other.ReadLock();
	WriteLock();
	mpf_init(m_value);
	mpf_set_z(m_value, other.m_value);
	Unlock();
	other.Unlock();
}

void MPFloat::SerializeOut(std::vector<char>& outbuffer) const
{
	mpz_t zpart;
	int expt = m_value->_mp_exp;
	size_t nz = m_value->_mp_size;
	unsigned char sign = (mpf_sgn(m_value) >= 0) ? 0 : 1;

	zpart->_mp_alloc = nz;
	zpart->_mp_size = nz;
	zpart->_mp_d = m_value->_mp_d;

	size_t countp = 0;
	char* data = reinterpret_cast<char*>(mpz_export(0, &countp, 1,
							1, 1, 0, zpart));
	if(data)
	{
		outbuffer.resize(countp + sizeof(int) + 1);
		memcpy(&outbuffer[0], &sign, sizeof(char));
		memcpy(&outbuffer[1], &expt, sizeof(int));
		memcpy(&outbuffer[sizeof(int) + 1], data, countp);
		free(data);
	}
}

void MPFloat::SerializeIn(const char* inbuffer, const size_t len)
{
	mpz_t temp;
	int exp = 0;
	unsigned char sign = 0;

	memcpy(&sign, inbuffer, sizeof(char));
	memcpy(&exp, inbuffer + 1, sizeof(int));

	mpz_init(temp);
	mpz_import(temp, len - 1 - sizeof(int),
		1, 1, 1, 0, inbuffer + (sizeof(int) + 1));
	mpf_set_z(m_value, temp);
	m_value->_mp_exp = exp;

	if(sign)
	{
		mpf_neg(m_value, m_value);
	}

	mpz_clear(temp);
}

const std::string MPFloat::toString(int digits)
{
	char buf[256] = {0};
	ReadLock();
	gmp_snprintf(buf, 256, "%.*Ff",
				    digits, m_value);
	std::string retval = buf;
	Unlock();
	return retval;
}

MPFloat* operator-(const unsigned int a, const MPFloat& mpnum)
{
	mpnum.ReadLock();
	MPFloat* temporary = MPFloat::Factory(0);
	mpf_ui_sub(temporary->m_value, a, mpnum.m_value);
	mpnum.Unlock();
	return temporary;
}

MPFloat& MPFloat::operator=(const MPFloat& other)
{
	dbgprintf("MPFloat copy assignment\n");
	other.ReadLock();
	WriteLock();
	mpf_set(m_value, other.m_value);
	AddRef();
	Unlock();
	other.Unlock();
	return *this;
}

MPFloat& MPFloat::operator=(const MPInt& other)
{
	other.ReadLock();
	WriteLock();
	mpf_init(m_value);
	mpf_set_z(m_value, other.m_value);
	AddRef();
	Unlock();
	other.Unlock();
	return *this;
}

MPFloat& MPFloat::operator=(const unsigned int num)
{
	dbgprintf("Unsigned int assignment\n");
	WriteLock();
	mpf_set_ui(m_value, num);
	AddRef();
	Unlock();
	return *this;
}

MPFloat& MPFloat::operator=(const int num)
{
	dbgprintf("Signed int assignment: %d\n", num);
	WriteLock();
	mpf_set_si(m_value, num);
	AddRef();
	Unlock();
	return *this;
}

MPFloat& MPFloat::operator=(const double num)
{
	WriteLock();
	dbgprintf("Double assignment.\n");
	mpf_set_d(m_value, num);
	AddRef();
	Unlock();
	return *this;
}

MPFloat& MPFloat::operator=(const std::string& str)
{
	WriteLock();
	mpf_set_str(m_value, str.c_str(), 10);
	Unlock();
	return *this;
}

MPFloat& MPFloat::operator+=(const MPFloat& other)
{
	WriteLock();
	other.ReadLock();
	mpf_add(m_value, m_value, other.m_value);
	AddRef();
	other.Unlock();
	Unlock();
	return *this;
}

MPFloat& MPFloat::operator+=(const unsigned int num)
{
	WriteLock();
	mpf_add_ui(m_value, m_value, num);
	AddRef();
	Unlock();
	return *this;
}

MPFloat& MPFloat::operator-=(const MPFloat& other)
{
	WriteLock();
	mpf_sub(m_value, m_value, other.m_value);
	AddRef();
	Unlock();
	return *this;
}

MPFloat& MPFloat::operator-=(const unsigned int num)
{
	WriteLock();
	mpf_sub_ui(m_value, m_value, num);
	AddRef();
	Unlock();
	return *this;
}

MPFloat& MPFloat::operator*=(const MPFloat& other)
{
	WriteLock();
	other.ReadLock();
	mpf_mul(m_value, m_value, other.m_value);
	AddRef();
	other.Unlock();
	Unlock();
	return *this;
}

MPFloat& MPFloat::operator*=(const unsigned int num)
{
	dbgprintf("Multiply-Assign with argument %u\n", num);
	WriteLock();
	mpf_mul_ui(m_value, m_value, num);
	AddRef();
	Unlock();
	return *this;
}

MPFloat& MPFloat::operator/=(const MPFloat& other)
{
	if(other == 0)
	{
		SetASException("Divide by zero exception.");
		AddRef();
		return *this;
	}
	WriteLock();
	other.ReadLock();
	mpf_div(m_value, m_value, other.m_value);
	other.Unlock();
	Unlock();
	return *this;
}

MPFloat& MPFloat::operator/=(const unsigned int num)
{
	if(num == 0)
	{
		SetASException("Divide by zero exception.");
		AddRef();
		return *this;
	}
	WriteLock();
	mpf_div_ui(m_value, m_value, num);
	Unlock();
	return *this;
}

MPFloat& MPFloat::powassign(const unsigned int power)
{
	WriteLock();
	mpf_pow_ui(m_value, m_value, power);
	Unlock();
	return *this;
}

bool MPFloat::operator==(const MPFloat &other) const
{
	ReadLock();
	other.ReadLock();
	bool result = 0 == mpf_cmp(m_value, other.m_value);
	other.Unlock();
	Unlock();
	return result;
}

bool MPFloat::operator==(const double num) const
{
	ReadLock();
	bool result = 0 == mpf_cmp_d(m_value, num);
	Unlock();
	return result;
}

bool MPFloat::operator==(const int num) const
{
	ReadLock();
	bool result = 0 == mpf_cmp_si(m_value, num);
	Unlock();
	return result;
}

bool MPFloat::operator==(const unsigned int num) const
{
	ReadLock();
	bool result = 0 == mpf_cmp_ui(m_value, num);
	Unlock();
	return result;
}

int MPFloat::opCmp(const MPFloat& other) const
{
	other.ReadLock();
	ReadLock();
	bool result = mpf_cmp(m_value, other.m_value);
	Unlock();
	other.Unlock();
	return result;
}

int MPFloat::opCmp(const double num) const
{
	ReadLock();
	int result = mpf_cmp_d(m_value, num);
	Unlock();
	return result;
}

int MPFloat::opCmp(const int num) const
{
	ReadLock();
	int result = mpf_cmp_si(m_value, num);
	Unlock();
	return result;
}

int MPFloat::opCmp(const unsigned int num) const
{
	ReadLock();
	int result = mpf_cmp_ui(m_value, num);
	Unlock();
	return result;
}

bool MPFloat::isSame(const MPFloat& other) const
{
	return &m_value == &other.m_value;
}

bool MPFloat::isNotSame(const MPFloat& other) const
{
	return !isSame(other);
}

MPFloat* MPFloat::operator+(const MPFloat& other)
{
	MPFloat* temporary = MPFloat::Factory(0);
	//Don't need to do any locking with the temporary,
	//since at this point only we have access to it here
	ReadLock();
	other.ReadLock();
	mpf_add(temporary->m_value, m_value, other.m_value);
	other.Unlock();
	Unlock();
	return temporary;
}

MPFloat* MPFloat::operator+(const unsigned int num)
{
	MPFloat* temporary = MPFloat::Factory(0);
	ReadLock();
	mpf_add_ui(temporary->m_value, m_value, num);
	Unlock();
	return temporary;
}

MPFloat* MPFloat::operator-()
{
	MPFloat* temporary = MPFloat::Factory(0);
	ReadLock();
	mpf_neg(temporary->m_value, m_value);
	Unlock();
	return temporary;
}

MPFloat* MPFloat::operator-(const MPFloat& other)
{
	MPFloat* temporary = MPFloat::Factory(0);
	ReadLock();
	other.ReadLock();
	mpf_sub(temporary->m_value, m_value, other.m_value);
	other.Unlock();
	Unlock();
	return temporary;
}

MPFloat* MPFloat::operator-(const unsigned int num)
{
	MPFloat* temporary = MPFloat::Factory(0);
	ReadLock();
	mpf_sub_ui(temporary->m_value, m_value, num);
	Unlock();
	return temporary;
}

MPFloat* MPFloat::operator*(const MPFloat& other)
{
	MPFloat* temporary = MPFloat::Factory(0);
	other.ReadLock();
	ReadLock();
	mpf_mul(temporary->m_value, m_value, other.m_value);
	Unlock();
	other.Unlock();
	return temporary;
}

MPFloat* MPFloat::operator*(const unsigned int num)
{
	MPFloat* temporary = MPFloat::Factory(0);
	ReadLock();
	mpf_mul_ui(temporary->m_value, m_value, num);
	Unlock();
	return temporary;
}

MPFloat* MPFloat::operator/(const MPFloat& other)
{
	if(other == 0)
	{
		SetASException("Divide by zero exception.");
		return 0;
	}

	MPFloat* temporary = MPFloat::Factory(0);
	ReadLock();
	other.ReadLock();
	mpf_div(temporary->m_value, m_value, other.m_value);
	other.Unlock();
	Unlock();
	return temporary;
}

MPFloat* MPFloat::operator/(const unsigned int num)
{
	if(!num)
	{
		SetASException("Divide by zero exception.");
		return 0;
	}

	MPFloat* temporary = MPFloat::Factory(0);
	ReadLock();
	mpf_div_ui(temporary->m_value, m_value, num);
	Unlock();
	return temporary;
}

MPFloat* MPFloat::pow(const unsigned int power)
{
	MPFloat* temporary = MPFloat::Factory(0);
	ReadLock();
	mpf_pow_ui(temporary->m_value, m_value, power);
	Unlock();
	return temporary;
}

MPFloat* MPFloat::Abs()
{
	MPFloat* temporary = MPFloat::Factory(0);
	ReadLock();
	mpf_abs(temporary->m_value, m_value);
	Unlock();
	return temporary;
}
