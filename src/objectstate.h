#ifndef OBJECTSTATE_H_
#define OBJECTSTATE_H_
#include <string>

#include "uuid.h"

#include "as_addons/scripthandle.h"

#include "sqlitetable.h"
#include "sqliterow.h"

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

	void LoadPropertyText(const std::string& name, std::string& v);
	void LoadPropertyUINT8(const std::string& name, unsigned char& v);
	void LoadPropertyUINT16(const std::string& name, unsigned short& v);
	void LoadPropertyUINT32(const std::string& name, unsigned int& v);
	void LoadPropertyUINT64(const std::string& name, unsigned long long& v);
	void LoadPropertyINT8(const std::string& name, char& v);
	void LoadPropertyINT16(const std::string& name, short& v);
	void LoadPropertyINT32(const std::string& name, int& v);
	void LoadPropertyINT64(const std::string& name, long long& v);
	void LoadPropertyFloat(const std::string& name, float& v);
	void LoadPropertyDouble(const std::string& name, double& v);
	void LoadPropertyBlob(const std::string& name, const char* data, size_t len); //Not to be used directly from scripts
	void LoadPropertyUUID(const std::string& name, UUID& uuid);
};

int RegisterObjectStateClass(asIScriptEngine* engine, struct Database* db);

#endif
