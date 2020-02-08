#include "persistentobj.h"
#include "angelscript.h"
#include <cstdio>
#include <cstring>

static const char* persistentobjscript =
	"shared abstract class PersistentObj {"
	"PersistentObj(){@m_obj = PersistentObj_t();}"
	"protected void SaveProperty(string name, string val){m_obj.SaveProperty(name, val);}"
	"PersistentObj_t @opImplCast() {return m_obj;}"
	"PersistentObj_t @m_obj;"
	"};";

void PersistentObj::SaveProperty(const std::string& propname, const std::string& propval)
{
	asITypeInfo* typeinfo = m_obj->GetObjectType();
	printf("SaveProperty called from instance of class %s with name: %s val %s\n",
		typeinfo->GetName(), propname.c_str(), propval.c_str());
}

std::string LoadStringProperty(const std::string& name, const std::string& defaultvalue)
{
	asITypeInfo* typeinfo = m_obj->GetObjectType();
	printf("LoadStringProperty\n");
}

PersistentObj::PersistentObj(asIScriptObject* obj)
: AS_RefCountedObj(obj)
{

}

PersistentObj::~PersistentObj()
{

}

int RegisterPersistentObjProxyClass(asIScriptEngine* engine, asIScriptModule* module)
{
	int result = 0;
	result = engine->RegisterObjectType("PersistentObj_t", 0, asOBJ_REF);
	if(result < 0)
		return -1;
	result = engine->RegisterObjectBehaviour("PersistentObj_t", asBEHAVE_FACTORY,
						"PersistentObj_t @f()", asFUNCTION(PersistentObj::Factory), asCALL_CDECL);
	if(result < 0)
		return -1;

	result = engine->RegisterObjectBehaviour("PersistentObj_t", asBEHAVE_ADDREF, "void f()", asMETHOD(PersistentObj, AddRef), asCALL_THISCALL);
	if(result < 0)
		return -1;

	result = engine->RegisterObjectBehaviour("PersistentObj_t", asBEHAVE_RELEASE, "void f()", asMETHOD(PersistentObj, Release), asCALL_THISCALL);
	if(result < 0)
		return -1;

	result = engine->RegisterObjectMethod("PersistentObj_t", "void SaveProperty(string& in, string& in)",
					asMETHODPR(PersistentObj, SaveProperty, (const std::string&, const std::string&), void), asCALL_THISCALL);
	if(result < 0)
		return -1;
	return 0;
}

int LoadPersistentObjScript(asIScriptEngine* engine, asIScriptModule* module)
{
	return module->AddScriptSection("game", persistentobjscript, strlen(persistentobjscript));
}

PersistentObj* PersistentObj::Factory()
{
	asIScriptContext* ctx = asGetActiveContext();
	asIScriptFunction* func = ctx->GetFunction(0);
	if( func->GetObjectType() == 0 || std::string(func->GetObjectType()->GetName()) != "PersistentObj" )
	{
		ctx->SetException("Invalid attempt to manually instantiate PersistentObj_t");
		return 0;
	}

	asIScriptObject *obj = reinterpret_cast<asIScriptObject*>(ctx->GetThisPointer(0));
	return new PersistentObj(obj);
}
