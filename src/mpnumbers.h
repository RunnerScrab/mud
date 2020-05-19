#ifndef MPNUMBERS_H_
#define MPNUMBERS_H_
#include <string>
#include "poolalloc.h"
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

class MemoryPoolAllocator
{
public:
	MemoryPoolAllocator()
	{
		MemoryPool_Init(&m_mempool, 64);
	}

	~MemoryPoolAllocator()
	{
		MemoryPool_Destroy(&m_mempool);
	}

	void* Alloc(size_t size)
	{
		return MemoryPool_Alloc(&m_mempool, size);
	}

	void Free(size_t size, void* p)
	{
		MemoryPool_Free(&m_mempool, size, p);
	}
private:
	MemoryPool m_mempool;
};

class MPInt:
	public AS_RefCountedObj
{
	static MemoryPoolAllocator m_static_mempool;
public:
	static MPInt* Factory(int initvalue)
	{
		dbgprintf("MPInt construction with value: %d\n", initvalue);
		//void* pMem = m_static_mempool.Alloc(sizeof(MPInt));
		//return new(pMem) MPInt(initvalue);
		return new MPInt(initvalue);
	}

/*	void operator delete(void* p)
	{
		dbgprintf("Freeing MPInt mempool slot.\n");
		m_static_mempool.Free(sizeof(MPInt), p);
	}
*/

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

#endif
