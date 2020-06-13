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
	return 0;
}

int MultiPrecisionLibrary_Teardown()
{
	return 0;
}


int RegisterMPNumberClasses(asIScriptEngine* engine)
{
	int result = 0;

	//Both types have operations which refer to the other, so
	//the types must be registered before their methods

	result = engine->RegisterObjectType("MPInt", sizeof(MPInt),
					    asOBJ_VALUE);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectType("MPFloat", sizeof(MPFloat),
					    asOBJ_VALUE);
	RETURNFAIL_IF(result < 0);

	result = RegisterMPIntClass(engine);
	RETURNFAIL_IF(result < 0);

	result = RegisterMPFloatClass(engine);

	return result;
}
