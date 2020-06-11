#ifndef MPFLOAT_H_
#define MPFLOAT_H_

#include <string>
#include "as_refcountedobj.h"
#include "rwlockingobj.h"
#include "mpnumbers.h"
#include "poolalloc.h"
#include "utils.h"
#include "mpint.h"
#include "gmp/gmp.h"

class MPInt;

class MPFloat:
	public AS_RefCountedObj, public RWLockingObject
{
	friend class MPInt;
	friend MPFloat* operator-(const unsigned int a, const MPFloat& mpnum);
public:
	static MemoryPoolAllocator* m_static_mempool;

	template <typename T> static MPFloat* Factory(const T initvalue)
	{
		if(m_static_mempool)
		{
			void* pMem = m_static_mempool->Alloc();
			return new(pMem) MPFloat(initvalue);
		}
		else
		{
			return 0;
		}
	}

/*	static MPFloat* Factory(const MPFloat& initvalue)
	{
		if(m_static_mempool)
		{
			void* pMem = m_static_mempool->Alloc();
			return new(pMem) MPFloat(initvalue);
		}
		else
		{
			return 0;
		}
	}
*/
	void operator delete(void* p)
	{
		if(m_static_mempool)
		{
			m_static_mempool->Free(p);
		}
	}

	MPFloat(const MPInt& other);

	MPFloat(const MPFloat& other)
	{
		other.ReadLock();
		WriteLock();
		mpf_init_set(m_value, other.m_value);
		Unlock();
		other.Unlock();
	}

	MPFloat(const int initval = 0)
	{
		WriteLock();
		mpf_init_set_si(m_value, initval);
		Unlock();
	}

	MPFloat(const unsigned int initval)
	{
		WriteLock();
		mpf_init_set_ui(m_value, initval);
		Unlock();
	}

	MPFloat(const double initval)
	{
		WriteLock();
		mpf_init_set_d(m_value, initval);
		Unlock();
	}

	~MPFloat()
	{
		mpf_clear(m_value);
	}

	//Assignment operators
	MPFloat& operator=(const MPFloat& other);
	MPFloat& operator=(const unsigned int num);
	MPFloat& operator=(const int num);
	MPFloat& operator=(const double num);

	//Assignment-arithmetic
	MPFloat& operator+=(const MPFloat& other);
	MPFloat& operator+=(const unsigned int num);
	MPFloat& operator-=(const MPFloat& other);
	MPFloat& operator-=(const unsigned int num);
	MPFloat& operator*=(const MPFloat& other);
	MPFloat& operator*=(const unsigned int num);
	MPFloat& operator/=(const MPFloat& other);
	MPFloat& operator/=(const unsigned int num);

	MPFloat& powassign(const unsigned int power);
	//Comparison operators
	bool operator==(const MPFloat &other) const;
	bool operator==(const double num) const;
	bool operator==(const int num) const;
	bool operator==(const unsigned int num) const;

	int opCmp(const MPFloat& other) const;
	int opCmp(const double num) const;
	int opCmp(const int num) const;
	int opCmp(const unsigned int num) const;

	bool isSame(const MPFloat& other) const; //Compares object references
	bool isNotSame(const MPFloat& other) const;

	//Arithmetic operators
	MPFloat* operator+(const MPFloat& other);
	MPFloat* operator+(const unsigned int num);

	MPFloat* operator-();
	MPFloat* operator-(const MPFloat& other);
	MPFloat* operator-(const unsigned int num);

	MPFloat* operator*(const MPFloat& other);
	MPFloat* operator*(const unsigned int num);
	MPFloat* operator*(const int num);

	MPFloat* operator/(const MPFloat& other);
	MPFloat* operator/(const unsigned int num);

	MPFloat* pow(const unsigned int power);
	MPFloat* Abs();

	//Conversion operators
	const std::string toString(int digits = 3);
private:
	mpf_t m_value;
};

int RegisterMPFloatClass(asIScriptEngine* engine);

MPFloat* operator-(const unsigned int a, const MPFloat& mpnum);


#endif
