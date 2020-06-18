#include "mpnumbers.h"
#include "mpint.h"
#include "angelscript.h"
#include "utils.h"
#include <vector>

void SetASException(const char* msg)
{
	asIScriptContext* ctx = asGetActiveContext();
	ctx->SetException(msg);
}

int MultiPrecisionLibrary_Init()
{
	MPInt::m_static_mempool = new MemoryPoolAllocator(sizeof(MPInt));
	MPFloat::m_static_mempool = new MemoryPoolAllocator(sizeof(MPFloat));
	return 0;
}

int MultiPrecisionLibrary_Teardown()
{
	delete MPInt::m_static_mempool;
	delete MPFloat::m_static_mempool;
	return 0;
}


int RegisterMPNumberClasses(asIScriptEngine* engine)
{
	int result = 0;

	//Both types have operations which refer to the other, so
	//the types must be registered before their methods

	result = engine->RegisterObjectType("MPInt", sizeof(MPInt),
					    asOBJ_VALUE | asGetTypeTraits<MPInt>());
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectType("MPFloat", sizeof(MPFloat),
					    asOBJ_VALUE | asGetTypeTraits<MPFloat>());
	RETURNFAIL_IF(result < 0);

	result = RegisterMPIntClass(engine);
	RETURNFAIL_IF(result < 0);

	result = RegisterMPFloatClass(engine);

	return result;
}
