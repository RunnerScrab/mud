#include "mpnumbers.h"
#include "mpint.h"
#include "angelscript.h"
#include "utils.h"
#include <vector>

MemoryPoolAllocator* g_static_mempool;

void SetASException(const char* msg)
{
	asIScriptContext* ctx = asGetActiveContext();
	ctx->SetException(msg);
}

int MultiPrecisionLibrary_Init()
{
	g_static_mempool = new MemoryPoolAllocator(sizeof(MPInt));
	MPInt::m_static_mempool = g_static_mempool;
	MPFloat::m_static_mempool = new MemoryPoolAllocator(sizeof(MPFloat));
	return 0;
}

int MultiPrecisionLibrary_Teardown()
{
	delete g_static_mempool;
	MPInt::m_static_mempool = 0;
	delete MPFloat::m_static_mempool;
	return 0;
}


int RegisterMPNumberClasses(asIScriptEngine* engine)
{
	int result = 0;

	result = engine->RegisterObjectType("MPInt", 0, asOBJ_REF);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectType("MPFloat", 0, asOBJ_REF);
	RETURNFAIL_IF(result < 0);

	result = RegisterMPIntClass(engine);
	RETURNFAIL_IF(result < 0);

	result = RegisterMPFloatClass(engine);

	return result;
}
