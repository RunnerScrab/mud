#ifndef MPNUMBERS_H_
#define MPNUMBERS_H_
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
		AS_RefCountedObj
{

public:
	static MPInt* Factory(long long int initvalue)
	{
		return new MPInt(initvalue);
	}

	MPInt(long long int initvalue = 0)
	{
		mpz_init(m_value);
		mpz_set_ui(m_value, initvalue);
	}

	~MPInt()
	{
		mpz_clear(m_value);
	}

	bool operator==(const MPInt &other)
	{
		return 0 == mpz_cmp(m_value, other.m_value);
	}

private:
	mpz_t m_value;
};

int RegisterMPNumberClasses(asIScriptEngine *engine);

#endif
