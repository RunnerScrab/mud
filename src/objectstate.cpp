#include "objectstate.h"
#include "angelscript.h"
#include "constants.h"
#include "as_addons/scripthandle.h"
#include "as_manager.h"
#include "database.h"
#include <cstdio>
#include <string>

struct Database* ObjectState::st_m_pDB = 0;

ObjectState::ObjectState(CScriptHandle obj) : m_refCount(1)
{
	m_objtypeinfo = obj.GetType();
	if(m_objtypeinfo)
	{
		printf("ObjectState for object type %s\n", m_objtypeinfo->GetName());
		void* pUserData = m_objtypeinfo->GetUserData(AS_USERDATA_TYPESCHEMA);
		if(!pUserData)
		{
			printf("No ObjectState user data for object type %s\n", m_objtypeinfo->GetName());
			m_objtypeinfo->SetUserData(
				new SQLiteTable(GetDBConnection()->pDB, m_objtypeinfo->GetName()),
				AS_USERDATA_TYPESCHEMA);
		}
	}
	else
	{
		asIScriptContext *ctx = asGetActiveContext();
		if( ctx )
			ctx->SetException("Exception: Could not determine object type in ObjectState constructor.");
	}
}

ObjectState::~ObjectState()
{

}

void ObjectState::AddRef()
{
	asAtomicInc(m_refCount);
}

void ObjectState::Release()
{
	asAtomicDec(m_refCount);
	if(!m_refCount)
	{
		delete this;
	}
}

void ObjectState::SaveState()
{
	printf("ObjectState::SaveState() called!\n");
}

ObjectState* ObjectState::Factory(CScriptHandle obj)
{
	return new ObjectState(obj);
}

void ObjectStateTypeInfoCleanup(asITypeInfo* pTypeInfo)
{
	void* pUserData = pTypeInfo->GetUserData(AS_USERDATA_TYPESCHEMA);
	if(!pUserData)
	{
		printf("No ObjectState user data for object type %s\n", pTypeInfo->GetName());
		return;
	}
	printf("Deleting allocated schema information for class %s\n", pTypeInfo->GetName());
	delete reinterpret_cast<SQLiteTable*>(pUserData);
}

int RegisterObjectStateClass(asIScriptEngine* engine, struct Database* db)
{
	int result = 0;

	//By default, SQLite uses serialized database connection access, which is safe to use from
	//multiple threads. A mutex in the engine would only serve to do the same thing.

	ObjectState::SetDBConnection(db);

	engine->SetTypeInfoUserDataCleanupCallback(ObjectStateTypeInfoCleanup, AS_USERDATA_TYPESCHEMA);

	if(engine->RegisterObjectType("ObjectState", 0, asOBJ_REF) < 0)
		return -1;
	if(engine->RegisterObjectBehaviour("ObjectState", asBEHAVE_FACTORY, "ObjectState@ f(ref@ obj)", asFUNCTION(ObjectState::Factory), asCALL_CDECL) < 0)
		return -1;
	if(engine->RegisterObjectBehaviour("ObjectState", asBEHAVE_ADDREF, "void f()", asMETHOD(ObjectState, AddRef), asCALL_THISCALL) < 0)
		return -1;
	if(engine->RegisterObjectBehaviour("ObjectState", asBEHAVE_RELEASE, "void f()", asMETHOD(ObjectState, Release), asCALL_THISCALL) < 0)
		return -1;
	if(engine->RegisterObjectMethod("ObjectState", "void SaveState()", asMETHOD(ObjectState, SaveState), asCALL_THISCALL) < 0)
		return -1;

	result = engine->RegisterObjectMethod("ObjectState", "void SaveProperty(string& in, string& in)",
					      asMETHODPR(ObjectState,
							 SavePropertyText,
							 (const std::string&, const std::string&), void),
					      asCALL_THISCALL);
	if(result < 0)
		return -1;

	result = engine->RegisterObjectMethod("ObjectState", "void SaveProperty(string& in, uint8& in)",
					      asMETHODPR(ObjectState,
							 SavePropertyUINT8,
							 (const std::string&, unsigned char), void),
					      asCALL_THISCALL);
	if(result < 0)
		return -1;

	if(engine->RegisterObjectMethod("ObjectState", "void SaveProperty(string& in, uint16 v)",
					asMETHODPR(ObjectState, SavePropertyUINT16, (const std::string&, unsigned short), void),
					asCALL_THISCALL) < 0)
		return -1;
	if(engine->RegisterObjectMethod("ObjectState", "void SaveProperty(string& in, uint v)",
					asMETHODPR(ObjectState, SavePropertyUINT32, (const std::string&, unsigned int), void),
					asCALL_THISCALL) < 0)
		return -1;
	if(engine->RegisterObjectMethod("ObjectState", "void SaveProperty(string& in, uint64 v)",
					asMETHODPR(ObjectState, SavePropertyUINT64, (const std::string&, unsigned long long), void),
					asCALL_THISCALL) < 0)
		return -1;

	if(engine->RegisterObjectMethod("ObjectState", "void SaveProperty(string& in, int8 v)",
					asMETHODPR(ObjectState, SavePropertyINT8, (const std::string&, char), void),
					asCALL_THISCALL) < 0)
		return -1;
	if(engine->RegisterObjectMethod("ObjectState", "void SaveProperty(string& in, int16 v)",
					asMETHODPR(ObjectState, SavePropertyINT16, (const std::string&, short), void),
					asCALL_THISCALL) < 0)
		return -1;
	if(engine->RegisterObjectMethod("ObjectState", "void SaveProperty(string& in, int v)",
					asMETHODPR(ObjectState, SavePropertyINT32, (const std::string&, int), void),
					asCALL_THISCALL) < 0)
		return -1;

	if(engine->RegisterObjectMethod("ObjectState", "void SaveProperty(string& in, int64 v)",
					asMETHODPR(ObjectState, SavePropertyINT64, (const std::string&, long long), void),
					asCALL_THISCALL) < 0)
		return -1;

	if(engine->RegisterObjectMethod("ObjectState", "void SaveProperty(string& in, float v)",
					asMETHODPR(ObjectState, SavePropertyFloat, (const std::string&, float), void),
					asCALL_THISCALL) < 0)
		return -1;

	if(engine->RegisterObjectMethod("ObjectState", "void SaveProperty(string& in, double v)",
					asMETHODPR(ObjectState, SavePropertyDouble, (const std::string&, double), void),
					asCALL_THISCALL) < 0)
		return -1;

	if(engine->RegisterObjectMethod("ObjectState", "void SaveProperty(string& in, uuid& in)",
					asMETHODPR(ObjectState, SavePropertyUUID, (const std::string&, const UUID&), void),
					asCALL_THISCALL) < 0)
		return -1;

	return 0;
}

void ObjectState::SavePropertyText(const std::string& propname, const std::string& propval)
{
	printf("SaveProperty called from instance of class %s with name: %s val %s\n",
	       m_objtypeinfo->GetName(), propname.c_str(), propval.c_str());
}

void ObjectState::SavePropertyBlob(const std::string& name, const char* data, size_t len)
{

}

void ObjectState::SavePropertyUUID(const std::string& name, const UUID& uuid)
{
	printf("SaveProperty (uuid) called from instance of class %s with name: %s val %s\n",
	       m_objtypeinfo->GetName(), name.c_str(), uuid.ToString().c_str());
}

void ObjectState::SavePropertyUINT8(const std::string& propname, unsigned char byte)
{
	printf("SaveProperty (uint8) called from instance of class %s with name: %s val %d\n",
	       m_objtypeinfo->GetName(), propname.c_str(), byte);
}

void ObjectState::SavePropertyUINT16(const std::string& propname, unsigned short v)
{
}

void ObjectState::SavePropertyUINT32(const std::string& propname, unsigned int v)
{
	printf("SaveProperty (uint) called from instance of class %s with name: %s val %d\n",
	       m_objtypeinfo->GetName(), propname.c_str(), v);

}

void ObjectState::SavePropertyUINT64(const std::string& propname, unsigned long long v)
{

}

void ObjectState::SavePropertyINT8(const std::string& propname, char byte)
{

	printf("SaveProperty (uint8) called from instance of class %s with name: %s val %d\n",
	       m_objtypeinfo->GetName(), propname.c_str(), byte);
}

void ObjectState::SavePropertyINT16(const std::string& propname, short v)
{

}

void ObjectState::SavePropertyINT32(const std::string& propname, int v)
{
	printf("SaveProperty (uint) called from instance of class %s with name: %s val %d\n",
	       m_objtypeinfo->GetName(), propname.c_str(), v);
}

void ObjectState::SavePropertyINT64(const std::string& propname, long long v)
{
	printf("SaveProperty (int64) called from instance of class %s with name: %s val %lld\n",
	       m_objtypeinfo->GetName(), propname.c_str(), v);
}

void ObjectState::SavePropertyFloat(const std::string& propname, float v)
{
	printf("SaveProperty (float) called from instance of class %s with name: %s val %f\n",
	       m_objtypeinfo->GetName(), propname.c_str(), v);

}

void ObjectState::SavePropertyDouble(const std::string& propname, double v)
{
	printf("SaveProperty (double) called from instance of class %s with name: %s val %f\n",
	       m_objtypeinfo->GetName(), propname.c_str(), v);
}

void ObjectState::LoadPropertyText(const std::string& name, std::string& out)
{

}

void ObjectState::LoadPropertyUINT8(const std::string& name, unsigned char& out)
{

}

void ObjectState::LoadPropertyUINT16(const std::string& name, unsigned short& out)
{

}

void ObjectState::LoadPropertyUINT32(const std::string& name, unsigned int& out)
{

}

void ObjectState::LoadPropertyUINT64(const std::string& name, unsigned long long& out)
{

}

void ObjectState::LoadPropertyINT8(const std::string& name, char& out)
{

}

void ObjectState::LoadPropertyINT16(const std::string& name, short& out)
{

}

void ObjectState::LoadPropertyINT32(const std::string& name, int& out)
{

}

void ObjectState::LoadPropertyINT64(const std::string& name, long long& out)
{

}

void ObjectState::LoadPropertyFloat(const std::string& name, float& out)
{

}

void ObjectState::LoadPropertyDouble(const std::string& name, double& out)
{

}

void ObjectState::LoadPropertyBlob(const std::string& name, const char* data, size_t len)
{
	//Not to be used directly from scripts
}

void ObjectState::LoadPropertyUUID(const std::string& name, UUID& uuid_out)
{

}
