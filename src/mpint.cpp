#include "mpint.h"
#include "angelscript.h"

MemoryPoolAllocator* MPInt::m_static_mempool;

int RegisterMPIntClass(asIScriptEngine* engine)
{
	int result = 0;
	//result = engine->RegisterObjectType("MPInt", sizeof(MPInt), asOBJ_VALUE);
	/*
	  result = engine->RegisterObjectBehaviour("val", asBEHAVE_CONSTRUCT, "void f()",
	  asFUNCTION(Constructor), asCALL_CDECL_OBJLAST);
	  RETURNFAIL_IF(result < 0);

	  result = engine->RegisterObjectBehaviour("val", asBEHAVE_DESTRUCT, "void f()",
	  asFUNCTION(Destructor), asCALL_CDECL_OBJLAST);
	  RETURNFAIL_IF(result < 0);
	*/
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
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectBehaviour("MPInt", asBEHAVE_FACTORY,
						 "MPInt@ f(const MPInt& in)",
						 asFUNCTIONPR(MPInt::Factory, (const MPInt&), MPInt*),
						 asCALL_CDECL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectBehaviour("MPInt", asBEHAVE_FACTORY,
						 "MPInt@ f(const MPFloat& in)",
						 asFUNCTIONPR(MPInt::Factory, (const MPFloat&), MPInt*),
						 asCALL_CDECL);
	RETURNFAIL_IF(result < 0);


	result = engine->RegisterObjectBehaviour("MPInt", asBEHAVE_FACTORY,
						 "MPInt@ f(const int32 num = 0)",
						 asFUNCTIONPR(MPInt::Factory, (const int), MPInt*),
						 asCALL_CDECL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectBehaviour("MPInt", asBEHAVE_FACTORY,
						 "MPInt@ f(const uint32 num)",
						 asFUNCTIONPR(MPInt::Factory, (const unsigned int), MPInt*),
						 asCALL_CDECL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectBehaviour("MPInt", asBEHAVE_FACTORY,
						 "MPInt@ f(const double num)",
						 asFUNCTIONPR(MPInt::Factory, (const double), MPInt*),
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

	//Arithmetic-assignment operators

	result = engine->RegisterObjectMethod("MPInt", "MPInt@ opAddAssign(const MPInt& in)",
					      asMETHODPR(MPInt, operator+=, (const MPInt&), MPInt&),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPInt", "MPInt@ opAddAssign(const uint32 num)",
					      asMETHODPR(MPInt, operator+=, (const unsigned int), MPInt&),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPInt", "MPInt@ opSubAssign(const MPInt& in)",
					      asMETHODPR(MPInt, operator-=, (const MPInt&), MPInt&),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPInt", "MPInt@ opSubAssign(const uint32 num)",
					      asMETHODPR(MPInt, operator-=, (const unsigned int), MPInt&),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPInt", "MPInt@ opMulAssign(const MPInt& in)",
					      asMETHODPR(MPInt, operator*=, (const MPInt&), MPInt&),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPInt", "MPInt@ opMulAssign(const uint32 num)",
					      asMETHODPR(MPInt, operator*=, (const unsigned int), MPInt&),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPInt", "MPInt@ opMulAssign(const int32 num)",
					      asMETHODPR(MPInt, operator*=, (const int), MPInt&),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPInt", "MPInt@ opDivAssign(const MPInt& in)",
					      asMETHODPR(MPInt, operator/=, (const MPInt&), MPInt&),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPInt", "MPInt@ opDivAssign(const uint32 num)",
					      asMETHODPR(MPInt, operator/=, (const unsigned int), MPInt&),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPInt", "MPInt@ opModAssign(const MPInt& in)",
					      asMETHODPR(MPInt, operator%=, (const MPInt&), MPInt&),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPInt", "MPInt@ opModAssign(const uint32 num)",
					      asMETHODPR(MPInt, operator%=, (const unsigned int), MPInt&),
					      asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPInt", "MPInt@ opPowAssign(const uint32 num)",
					      asMETHODPR(MPInt, powassign, (const unsigned int), MPInt&),
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


MPInt::MPInt(const MPFloat& other)
{
	other.ReadLock();
	WriteLock();
	mpz_init(m_value);
	mpz_set_f(m_value, other.m_value);
	Unlock();
	other.Unlock();
}

const std::string MPInt::toString()
{
	ReadLock();
	size_t minsize = mpz_sizeinbase(m_value, 10) + 2;
	std::string retval;
	retval.resize(minsize);
	mpz_get_str(&retval[0], 10, m_value);
	retval.resize(retval.find('\0'));
	dbgprintf("Converting mpint to %s\n", retval.c_str());
	Unlock();
	return retval;
}

MPInt* operator-(const unsigned int a, const MPInt& mpnum)
{
	mpnum.ReadLock();
	MPInt* temporary = MPInt::Factory(0);
	mpz_ui_sub(temporary->m_value, a, mpnum.m_value);
	mpnum.Unlock();
	return temporary;
}

MPInt& MPInt::operator=(const MPInt& other)
{
	dbgprintf("MPInt copy assignment\n");
	other.ReadLock();
	WriteLock();
	mpz_set(m_value, other.m_value);
	AddRef();
	Unlock();
	other.Unlock();
	return *this;
}

MPInt& MPInt::operator=(const unsigned int num)
{
	dbgprintf("Unsigned int assignment\n");
	WriteLock();
	mpz_set_ui(m_value, num);
	AddRef();
	Unlock();
	return *this;
}

MPInt& MPInt::operator=(const int num)
{
	dbgprintf("Signed int assignment: %d\n", num);
	WriteLock();
	mpz_set_si(m_value, num);
	AddRef();
	Unlock();
	return *this;
}

MPInt& MPInt::operator=(const double num)
{
	WriteLock();
	dbgprintf("Double assignment.\n");
	mpz_set_d(m_value, num);
	AddRef();
	Unlock();
	return *this;
}

MPInt& MPInt::operator+=(const MPInt& other)
{
	WriteLock();
	other.ReadLock();
	mpz_add(m_value, m_value, other.m_value);
	AddRef();
	other.Unlock();
	Unlock();
	return *this;
}

MPInt& MPInt::operator+=(const unsigned int num)
{
	WriteLock();
	mpz_add_ui(m_value, m_value, num);
	AddRef();
	Unlock();
	return *this;
}

MPInt& MPInt::operator-=(const MPInt& other)
{
	WriteLock();
	mpz_sub(m_value, m_value, other.m_value);
	AddRef();
	Unlock();
	return *this;
}

MPInt& MPInt::operator-=(const unsigned int num)
{
	WriteLock();
	mpz_sub_ui(m_value, m_value, num);
	AddRef();
	Unlock();
	return *this;
}

MPInt& MPInt::operator*=(const MPInt& other)
{
	WriteLock();
	other.ReadLock();
	mpz_mul(m_value, m_value, other.m_value);
	AddRef();
	other.Unlock();
	Unlock();
	return *this;
}

MPInt& MPInt::operator*=(const unsigned int num)
{
	dbgprintf("Multiply-Assign with argument %u\n", num);
	WriteLock();
	mpz_mul_ui(m_value, m_value, num);
	AddRef();
	Unlock();
	return *this;
}

MPInt& MPInt::operator*=(const int num)
{
	WriteLock();
	mpz_mul_si(m_value, m_value, num);
	AddRef();
	Unlock();
	return *this;
}

MPInt& MPInt::operator%=(const MPInt& other)
{
	if(other == 0)
	{
		SetASException("Divide by zero exception.");
		AddRef();
		return *this;
	}
	other.ReadLock();
	WriteLock();
	AddRef();
	mpz_tdiv_r(m_value, m_value, other.m_value);
	Unlock();
	other.Unlock();
	return *this;
}

MPInt& MPInt::operator%=(const unsigned int num)
{
	if(num == 0)
	{
		SetASException("Divide by zero exception.");
		AddRef();
		return *this;
	}
	WriteLock();
	mpz_tdiv_r_ui(m_value, m_value, num);
	Unlock();
	return *this;
}

MPInt& MPInt::operator/=(const MPInt& other)
{
	if(other == 0)
	{
		SetASException("Divide by zero exception.");
		AddRef();
		return *this;
	}
	WriteLock();
	other.ReadLock();
	mpz_tdiv_q(m_value, m_value, other.m_value);
	other.Unlock();
	Unlock();
	return *this;
}

MPInt& MPInt::operator/=(const unsigned int num)
{
	if(num == 0)
	{
		SetASException("Divide by zero exception.");
		AddRef();
		return *this;
	}
	WriteLock();
	mpz_tdiv_q_ui(m_value, m_value, num);
	Unlock();
	return *this;
}

MPInt& MPInt::powassign(const unsigned int power)
{
	WriteLock();
	mpz_pow_ui(m_value, m_value, power);
	Unlock();
	return *this;
}

bool MPInt::operator==(const MPInt &other) const
{
	ReadLock();
	other.ReadLock();
	bool result = 0 == mpz_cmp(m_value, other.m_value);
	other.Unlock();
	Unlock();
	return result;
}

bool MPInt::operator==(const double num) const
{
	ReadLock();
	bool result = 0 == mpz_cmp_d(m_value, num);
	Unlock();
	return result;
}

bool MPInt::operator==(const int num) const
{
	ReadLock();
	bool result = 0 == mpz_cmp_si(m_value, num);
	Unlock();
	return result;
}

bool MPInt::operator==(const unsigned int num) const
{
	ReadLock();
	bool result = 0 == mpz_cmp_ui(m_value, num);
	Unlock();
	return result;
}

int MPInt::opCmp(const MPInt& other) const
{
	other.ReadLock();
	ReadLock();
	bool result = mpz_cmp(m_value, other.m_value);
	Unlock();
	other.Unlock();
	return result;
}

int MPInt::opCmp(const double num) const
{
	ReadLock();
	int result = mpz_cmp_d(m_value, num);
	Unlock();
	return result;
}

int MPInt::opCmp(const int num) const
{
	ReadLock();
	int result = mpz_cmp_si(m_value, num);
	Unlock();
	return result;
}

int MPInt::opCmp(const unsigned int num) const
{
	ReadLock();
	int result = mpz_cmp_ui(m_value, num);
	Unlock();
	return result;
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
	//Don't need to do any locking with the temporary,
	//since at this point only we have access to it here
	ReadLock();
	other.ReadLock();
	mpz_add(temporary->m_value, m_value, other.m_value);
	other.Unlock();
	Unlock();
	return temporary;
}

MPInt* MPInt::operator+(const unsigned int num)
{
	MPInt* temporary = MPInt::Factory(0);
	ReadLock();
	mpz_add_ui(temporary->m_value, m_value, num);
	Unlock();
	return temporary;
}

MPInt* MPInt::operator-()
{
	MPInt* temporary = MPInt::Factory(0);
	ReadLock();
	mpz_neg(temporary->m_value, m_value);
	Unlock();
	return temporary;
}

MPInt* MPInt::operator-(const MPInt& other)
{
	MPInt* temporary = MPInt::Factory(0);
	ReadLock();
	other.ReadLock();
	mpz_sub(temporary->m_value, m_value, other.m_value);
	other.Unlock();
	Unlock();
	return temporary;
}

MPInt* MPInt::operator-(const unsigned int num)
{
	MPInt* temporary = MPInt::Factory(0);
	ReadLock();
	mpz_sub_ui(temporary->m_value, m_value, num);
	Unlock();
	return temporary;
}

MPInt* MPInt::operator*(const MPInt& other)
{
	MPInt* temporary = MPInt::Factory(0);
	other.ReadLock();
	ReadLock();
	mpz_mul(temporary->m_value, m_value, other.m_value);
	Unlock();
	other.Unlock();
	return temporary;
}

MPInt* MPInt::operator*(const unsigned int num)
{
	MPInt* temporary = MPInt::Factory(0);
	ReadLock();
	mpz_mul_ui(temporary->m_value, m_value, num);
	Unlock();
	return temporary;
}

MPInt* MPInt::operator*(const int num)
{
	MPInt* temporary = MPInt::Factory(0);
	ReadLock();
	mpz_mul_si(temporary->m_value, m_value, num);
	Unlock();
	return temporary;
}

MPInt* MPInt::operator/(const MPInt& other)
{
	if(other == 0)
	{
		SetASException("Divide by zero exception.");
		return 0;
	}

	MPInt* temporary = MPInt::Factory(0);
	ReadLock();
	other.ReadLock();
	mpz_tdiv_q(temporary->m_value, m_value, other.m_value);
	other.Unlock();
	Unlock();
	return temporary;
}

MPInt* MPInt::operator/(const unsigned int num)
{
	if(!num)
	{
		SetASException("Divide by zero exception.");
		return 0;
	}

	MPInt* temporary = MPInt::Factory(0);
	ReadLock();
	mpz_tdiv_q_ui(temporary->m_value, m_value, num);
	Unlock();
	return temporary;
}

MPInt* MPInt::operator%(const MPInt& other)
{
	if(other == 0)
	{
		SetASException("Divide by zero exception.");
		return 0;
	}

	MPInt* temporary = MPInt::Factory(0);
	other.ReadLock();
	ReadLock();
	mpz_tdiv_r(temporary->m_value, m_value, other.m_value);
	Unlock();
	other.Unlock();
	return temporary;
}
MPInt* MPInt::operator%(const unsigned int num)
{
	if(num == 0)
	{
		SetASException("Divide by zero exception.");
		return 0;
	}

	MPInt* temporary = MPInt::Factory(0);
	ReadLock();
	mpz_tdiv_r_ui(temporary->m_value, m_value, num);
	Unlock();
	return temporary;
}

MPInt* MPInt::pow(const unsigned int power)
{
	MPInt* temporary = MPInt::Factory(0);
	ReadLock();
	mpz_pow_ui(temporary->m_value, m_value, power);
	Unlock();
	return temporary;
}

MPInt* MPInt::Abs()
{
	MPInt* temporary = MPInt::Factory(0);
	ReadLock();
	mpz_abs(temporary->m_value, m_value);
	Unlock();
	return temporary;
}
