#ifndef MPNUMBERS_H_
#define MPNUMBERS_H_
#include <string>
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
	static MPInt* Factory(long int initvalue)
	{
		return new MPInt(initvalue);
	}

	MPInt(long int initvalue = 0)
	{
		mpz_init(m_value);
		mpz_set_ui(m_value, initvalue);
	}

	~MPInt()
	{
		mpz_clear(m_value);
	}

	//Assignment operators
	MPInt& operator=(const MPInt& other)
	{
		mpz_set(m_value, other.m_value);
		return *this;
	}

	MPInt& operator=(const unsigned int num)
	{
		mpz_set_ui(m_value, num);
		return *this;
	}

	MPInt& operator=(const long int num)
	{
		mpz_set_si(m_value, num);
		return *this;
	}

	MPInt& operator=(const double num)
	{
		mpz_set_d(m_value, num);
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
	const std::string toString()
	{
		std::string retval;
		size_t minsize = mpz_sizeinbase(m_value, 10) + 2;
		retval.resize(minsize);
		mpz_get_str(&retval[0], 10, m_value);
		return retval;
	}
private:
	mpz_t m_value;
};

int RegisterMPIntClass(asIScriptEngine* engine);
int RegisterMPNumberClasses(asIScriptEngine *engine);

#endif
