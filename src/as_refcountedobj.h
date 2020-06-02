#ifndef AS_REFCOUNTEDOBJ_H_
#define AS_REFCOUNTEDOBJ_H_

class asIScriptObject;
class asILockableSharedBool;

class AS_RefCountedProxiedObj
{
	/*
	 * This is for classes which use a proxy object
	 * to allow script classes to inherit from them (indirectly through
	 * the proxy object).
	 */
public:
	void AddRef();
	void Release();
	asILockableSharedBool* GetWeakRefFlag()
	{
		return m_isDead;
	}
protected:
	AS_RefCountedProxiedObj(asIScriptObject *obj);
	virtual ~AS_RefCountedProxiedObj();
	int m_refCount;
	asILockableSharedBool *m_isDead;
	asIScriptObject *m_obj;
};

class AS_RefCountedObj
{
	/*
	 * This is for classes which are to be exposed directly
	 * to Angelscript
	 */
public:
	AS_RefCountedObj();
	virtual ~AS_RefCountedObj();
	void AddRef();
	void Release();
	asILockableSharedBool* GetWeakRefFlag();
protected:
	int m_refcount;
	asILockableSharedBool* m_weakrefflag;
};

#endif
