#include "mpnumbers.h"
#include "angelscript.h"
#include "utils.h"

int RegisterMPIntClass(asIScriptEngine* engine)
{
	int result = 0;
	result = engine->RegisterObjectType("MPInt", 0, asOBJ_REF);
	RETURNFAIL_IF(result < 0);

	//Reference and memory management
	result = engine->RegisterObjectBehaviour("MPInt", asBEHAVE_ADDREF, "void f()",
						asMETHOD(MPInt, AddRef), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);
	result = engine->RegisterObjectBehaviour("MPInt", asBEHAVE_RELEASE, "void f()",
						asMETHOD(MPInt, Release), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);
	result = engine->RegisterObjectBehaviour("MPInt", asBEHAVE_GET_WEAKREF_FLAG,
						"int& f()", asMETHOD(MPInt, GetWeakRefFlag),
						asCALL_THISCALL);
	result = engine->RegisterObjectBehaviour("MPInt", asBEHAVE_FACTORY,
						"MPInt@ f(int num = 0)", asFUNCTION(MPInt::Factory),
						asCALL_CDECL);
	RETURNFAIL_IF(result < 0);

	//Assignment operators

	result = engine->RegisterObjectMethod("MPInt", "MPInt@ opAssign(const MPInt &in)",
					asMETHODPR(MPInt, operator=, (const MPInt&), MPInt&),
					asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPInt", "MPInt@ opAssign(const uint32 n)",
					asMETHODPR(MPInt, operator=, (const unsigned int), MPInt&),
					asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);


	result = engine->RegisterObjectMethod("MPInt", "MPInt@ opAssign(const int n)",
					asMETHODPR(MPInt, operator=, (const long int), MPInt&),
					asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("MPInt", "MPInt@ opAssign(const double n)",
					asMETHODPR(MPInt, operator=, (const double), MPInt&),
					asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	/*
	result = engine->RegisterObjectMethod("MPInt", "MPInt@ opAssign(const double n)",
					asMETHODPR(MPInt, operator=, (const double), MPInt&),
					asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);
	*/

	//Comparison operators

	//Conversion operations
	result = engine->RegisterObjectMethod("MPInt", "string toString()",
					asMETHOD(MPInt, toString), asCALL_THISCALL);
	return result;
}

int RegisterMPNumberClasses(asIScriptEngine* engine)
{
	int result = 0;
	result = RegisterMPIntClass(engine);
	return result;
}
