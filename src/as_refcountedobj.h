#ifndef AS_REFCOUNTEDOBJ_H_
#define AS_REFCOUNTEDOBJ_H_

class asIScriptObject;
class asILockableSharedBool;

class AS_RefCountedObj
{
public:
	void AddRef();
	void Release();

protected:
	AS_RefCountedObj(asIScriptObject *obj);
	virtual ~AS_RefCountedObj();
	int m_refCount;
	asILockableSharedBool *m_isDead;
	asIScriptObject *m_obj;
};

#endif
