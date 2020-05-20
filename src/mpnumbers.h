#ifndef MPNUMBERS_H_
#define MPNUMBERS_H_

#ifdef __cplusplus
#include <string>
#include "as_refcountedobj.h"
#endif

#include "poolalloc.h"
#include "utils.h"
#include "gmp/gmp.h"


/*
  Angelscript bindings to gmp multiprecision library. We bind
  the C interface directly, as Angelscript objects need special
  handling for reference counting
*/
#ifdef __cplusplus
class asIScriptEngine;
class asILockableSharedBool;

class MemoryPoolAllocator
{
public:
	MemoryPoolAllocator(size_t size)
	{
		AllocPool_Init(&m_allocpool, 64, size);
	}

	~MemoryPoolAllocator()
	{
		AllocPool_Destroy(&m_allocpool);
	}

	void* Alloc()
	{
		return AllocPool_Alloc(&m_allocpool);
	}

	void Free(void* p)
	{
		AllocPool_Free(&m_allocpool, p);
	}
private:
	AllocPool m_allocpool;
};

class MPInt:
	public AS_RefCountedObj
{
public:
	static MemoryPoolAllocator* m_static_mempool;

	static MPInt* Factory(int initvalue)
	{
		if(m_static_mempool)
		{
			dbgprintf("MPInt construction with value: %d\n", initvalue);
			void* pMem = m_static_mempool->Alloc();
			return new(pMem) MPInt(initvalue);
		}
		else
		{
			return 0;
		}
	}

	void operator delete(void* p)
	{
		dbgprintf("Freeing MPInt mempool slot.\n");
		if(m_static_mempool)
		{
			m_static_mempool->Free(p);
		}
	}


	MPInt(int initvalue = 0)
	{
		mpz_init_set_si(m_value, initvalue);
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

	//Arithmetic operators
	MPInt* operator+(const MPInt& other)
	{
		MPInt* temporary = MPInt::Factory(0);
		mpz_add(temporary->m_value, m_value, other.m_value);
		return temporary;
	}

	MPInt* operator-()
	{
		MPInt* temporary = MPInt::Factory(0);
		mpz_neg(temporary->m_value, m_value);
		return temporary;
	}

	MPInt* operator-(const MPInt& other)
	{
		MPInt* temporary = MPInt::Factory(0);
		mpz_sub(temporary->m_value, m_value, other.m_value);
		return temporary;
	}

	MPInt* operator*(const MPInt& other)
	{
		MPInt* temporary = MPInt::Factory(0);
		mpz_mul(temporary->m_value, m_value, other.m_value);
		return temporary;
	}

	MPInt* Abs()
	{
		MPInt* temporary = MPInt::Factory(0);
		mpz_abs(temporary->m_value, m_value);
		return temporary;
	}

	//Conversion operators
	const std::string toString();
private:
	mpz_t m_value;
};

int RegisterMPIntClass(asIScriptEngine* engine);
int RegisterMPNumberClasses(asIScriptEngine *engine);

extern "C"
{
#endif
	int MultiPrecisionLibrary_Init();
	int MultiPrecisionLibrary_Teardown();
#ifdef __cplusplus
}
#endif
#endif
