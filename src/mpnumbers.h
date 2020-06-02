#ifndef MPNUMBERS_H_
#define MPNUMBERS_H_

#ifdef __cplusplus
#include <string>
#include "as_refcountedobj.h"
#include "rwlockingobj.h"
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

int RegisterMPNumberClasses(asIScriptEngine *engine);
void SetASException(const char* msg);

extern "C"
{
#endif
	int MultiPrecisionLibrary_Init();
	int MultiPrecisionLibrary_Teardown();
#ifdef __cplusplus
}
#endif
#endif
