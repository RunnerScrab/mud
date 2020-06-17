#ifndef MPINT_H_
#define MPINT_H_
#include <vector>
#include <string>
#include "as_refcountedobj.h"
#include "rwlockingobj.h"
#include "mpnumbers.h"
#include "mpfloat.h"
#include "poolalloc.h"
#include "utils.h"
#include "gmp/gmp.h"


class MPFloat;
/*
  Angelscript bindings to gmp multiprecision library. We bind
  the C interface directly, as Angelscript objects need special
  handling for reference counting
*/

class MPInt:
	public AS_RefCountedObj, public RWLockingObject
{
	friend class MPFloat;
	friend MPInt* operator-(const unsigned int a, const MPInt& mpnum);
public:
	static MemoryPoolAllocator* m_static_mempool;

	template <typename T> static MPInt* Factory(const T initvalue)
	{
		if(m_static_mempool)
		{
			//void* pMem = m_static_mempool->Alloc();
			return new MPInt(initvalue);
		}
		else
		{
			return 0;
		}
	}

	static void ASConstructor(void* pMem)
	{
		new(pMem) MPInt(0);
	}

	static void ASDestructor(void* pMem)
	{
		reinterpret_cast<MPInt*>(pMem)->~MPInt();
	}

	MPInt(const MPFloat& other);

	MPInt(const MPInt& other)
	{
		other.ReadLock();
		WriteLock();
		mpz_init_set(m_value, other.m_value);
		Unlock();
		other.Unlock();
	}

	MPInt(const int initval = 0)
	{
		WriteLock();
		mpz_init_set_si(m_value, initval);
		Unlock();
	}

	MPInt(const unsigned int initval)
	{
		WriteLock();
		mpz_init_set_ui(m_value, initval);
		Unlock();
	}

	MPInt(const double initval)
	{
		WriteLock();
		mpz_init_set_d(m_value, initval);
		Unlock();
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
	MPInt& operator=(const MPFloat& other);
	MPInt& operator=(const std::string& str);
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
	MPInt& powassign(const unsigned int power);
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

	void SerializeOut(std::vector<char>& outbuffer) const;
	void SerializeIn(const char* inbuffer, const size_t len);
private:
	mpz_t m_value;
};

int RegisterMPIntClass(asIScriptEngine* engine);

MPInt* operator-(const unsigned int a, const MPInt& mpnum);


#endif
