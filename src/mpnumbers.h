#ifndef MPNUMBERS_H_
#define MPNUMBERS_H_
#include <string>
#include "utils.h"
#include "gmp/gmp.h"
#include "as_refcountedobj.h"

/*
  Angelscript bindings to gmp multiprecision library. We bind
  the C interface directly, as Angelscript objects need special
  handling for reference counting
*/

class asIScriptEngine;
class asILockableSharedBool;

class MPInt:
	public AS_RefCountedObj
{

public:
	static MPInt* Factory(int initvalue)
	{
		dbgprintf("MPInt construction with value: %d\n", initvalue);
		return new MPInt(initvalue);
	}

	MPInt(int initvalue = 0)
	{
		mpz_init(m_value);
		mpz_set_si(m_value, initvalue);
	}

	~MPInt()
	{
		dbgprintf("Destroying mpint\n");
		mpz_clear(m_value);
	}

	//Assignment operators
	MPInt& operator=(const MPInt& other)
	{
		dbgprintf("MPInt copy assignment\n");
		mpz_set(m_value, other.m_value);
		AddRef();
		return *this;
	}

	MPInt& operator=(const unsigned int num)
	{
		dbgprintf("Unsigned int assignment\n");
		mpz_set_ui(m_value, num);
		AddRef();
		return *this;
	}

	MPInt& operator=(const int num)
	{
		dbgprintf("Signed int assignment: %d\n", num);
		mpz_set_si(m_value, num);
		AddRef();
		return *this;
	}

	MPInt& operator=(const double num)
	{
		dbgprintf("Double assignment.\n");
		mpz_set_d(m_value, num);
		AddRef();
		return *this;
	}

	//Comparison operators
	bool operator==(const MPInt &other)
	{
		return 0 == mpz_cmp(m_value, other.m_value);
	}

	bool operator!=(const MPInt &other)
	{
		return 0;
	}

	bool operator>(const MPInt &other)
	{
		return mpz_cmp(m_value, other.m_value) > 0;
	}

	bool operator<(const MPInt &other)
	{
		return mpz_cmp(m_value, other.m_value) < 0;
	}

	bool operator>=(const MPInt &other)
	{
		return mpz_cmp(m_value, other.m_value) >= 0;
	}

	bool operator<=(const MPInt &other)
	{
		return mpz_cmp(m_value, other.m_value) <= 0;
	}

	bool isSame(const MPInt& other)
	{
		return &m_value == &other.m_value;
	}

	bool isNotSame(const MPInt& other)
	{
		return !isSame(other);
	}

	//Conversion operators
	const std::string toString();
private:
	mpz_t m_value;
};

int RegisterMPIntClass(asIScriptEngine* engine);
int RegisterMPNumberClasses(asIScriptEngine *engine);

#endif
