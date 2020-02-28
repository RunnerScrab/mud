#ifndef OBJECTSTATE_H_
#define OBJECTSTATE_H_
#include <string>

#include "uuid.h"

#include "as_addons/scripthandle.h"

class asIScriptObject;
class asIScriptEngine;
struct Database;

class ObjectState
{
private:
	int m_refCount;
	asITypeInfo* m_objtypeinfo;
	static struct Database* st_m_pDB;
public:
	static void SetDBConnection(struct Database* db)
	{
		st_m_pDB = db;
	}

	static struct Database* GetDBConnection()
	{
		return st_m_pDB;
	}

	ObjectState(CScriptHandle obj);
	virtual ~ObjectState();

	void AddRef();
	void Release();

	void SaveState();
	static ObjectState* Factory(CScriptHandle obj);

	void SavePropertyText(const std::string& name, const std::string&);
	void SavePropertyUINT8(const std::string& name, unsigned char);
	void SavePropertyUINT16(const std::string& name, unsigned short);
	void SavePropertyUINT32(const std::string& name, unsigned int);
	void SavePropertyUINT64(const std::string& name, unsigned long long);
	void SavePropertyINT8(const std::string& name, char);
	void SavePropertyINT16(const std::string& name, short);
	void SavePropertyINT32(const std::string& name, int);
	void SavePropertyINT64(const std::string& name, long long);
	void SavePropertyFloat(const std::string& name, float);
	void SavePropertyDouble(const std::string& name, double);
	void SavePropertyBlob(const std::string& name, const char* data, size_t len);
	void SavePropertyUUID(const std::string& name, const UUID& uuid);

};

int RegisterObjectStateClass(asIScriptEngine* engine, struct Database* db);

#endif
