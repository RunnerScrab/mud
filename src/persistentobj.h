#ifndef PERSISTENTOBJ_H_
#define PERSISTENTOBJ_H_
#include "as_refcountedobj.h"
#include <string>

class asIScriptObject;
class asIScriptEngine;
class asIScriptModule;

class PersistentObj : public AS_RefCountedObj
{
public:
	void SaveProperty(const std::string& name, const std::string&);
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

	std::string LoadStringProperty(const std::string& name, const std::string& defaultvalue);
	static PersistentObj* Factory();
protected:

	PersistentObj(asIScriptObject* obj);
	~PersistentObj();
};

int RegisterPersistentObjProxyClass(asIScriptEngine* engine, asIScriptModule* module);
int LoadPersistentObjScript(asIScriptEngine* engine, asIScriptModule* module);
#endif
