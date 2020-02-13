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
	void SaveProperty(const std::string&, const std::string&);


	std::string LoadStringProperty(const std::string& name, const std::string& defaultvalue);
	static PersistentObj* Factory();
protected:

	PersistentObj(asIScriptObject* obj);
	~PersistentObj();
};

int RegisterPersistentObjProxyClass(asIScriptEngine* engine, asIScriptModule* module);
int LoadPersistentObjScript(asIScriptEngine* engine, asIScriptModule* module);
#endif
