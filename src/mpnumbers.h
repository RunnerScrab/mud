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

//What is really needed is a pool of mp numbers which have already been allocated
//and only need to be cleared to reuse. The structs' functions internally perform dynamic allocation
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
	friend MPInt* operator-(const unsigned int a, const MPInt& mpnum);
public:
	static MemoryPoolAllocator* m_static_mempool;

	static MPInt* Factory(int initvalue)
	{
		if(m_static_mempool)
		{
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
		mpz_clear(m_value);
	}

	//Assignment operators
	MPInt& operator=(const MPInt& other);
	MPInt& operator=(const unsigned int num);
	MPInt& operator=(const int num);
	MPInt& operator=(const double num);

	//Assignment-arithmetic
	MPInt& operator+=(const MPInt& other);
	MPInt& operator+=(const unsigned int num);
	MPInt& operator-=(const MPInt& other);
	MPInt& operator-=(const unsigned int num);
	MPInt& operator*=(const MPInt& other);
	MPInt& operator*=(const unsigned int num);
	MPInt& operator*=(const int num);
	MPInt& operator/=(const MPInt& other);
	MPInt& operator/=(const unsigned int num);
	MPInt& operator%=(const MPInt& other);
	MPInt& operator%=(const unsigned int);

	//Comparison operators
	bool operator==(const MPInt &other) const;
	bool operator==(const double num) const;
	bool operator==(const int num) const;
	bool operator==(const unsigned int num) const;

	int opCmp(const MPInt& other) const;
	int opCmp(const double num) const;
	int opCmp(const int num) const;
	int opCmp(const unsigned int num) const;

	bool isSame(const MPInt& other) const; //Compares object references
	bool isNotSame(const MPInt& other) const;

	//Arithmetic operators
	MPInt* operator+(const MPInt& other);
	MPInt* operator+(const unsigned int num);

	MPInt* operator-();
	MPInt* operator-(const MPInt& other);
	MPInt* operator-(const unsigned int num);

	MPInt* operator*(const MPInt& other);
	MPInt* operator*(const unsigned int num);
	MPInt* operator*(const int num);

	MPInt* operator/(const MPInt& other);
	MPInt* operator/(const unsigned int num);

	MPInt* operator%(const MPInt& other);
	MPInt* operator%(const unsigned int);
	MPInt* pow(const unsigned int power);
	MPInt* Abs();

	//Conversion operators
	const std::string toString();
private:
	mpz_t m_value;
};

int RegisterMPIntClass(asIScriptEngine* engine);
int RegisterMPNumberClasses(asIScriptEngine *engine);

MPInt* operator-(const unsigned int a, const MPInt& mpnum);

extern "C"
{
#endif
	int MultiPrecisionLibrary_Init();
	int MultiPrecisionLibrary_Teardown();
#ifdef __cplusplus
}
#endif
#endif
