#include "as_refcountedobj.h"
#include "angelscript.h"
#include "as_manager.h"
#include <cstring>
#include <string>

AS_RefCountedProxiedObj::AS_RefCountedProxiedObj(asIScriptObject *obj)
{
	m_obj = obj;
	m_refCount = 1;
	m_isDead = obj->GetWeakRefFlag();
	m_isDead->AddRef();
}

AS_RefCountedProxiedObj::~AS_RefCountedProxiedObj()
{
	m_isDead->Release();
}

void AS_RefCountedProxiedObj::AddRef()
{
	asAtomicInc(m_refCount);
	if (!m_isDead->Get())
	{
		m_obj->AddRef();
	}
}

void AS_RefCountedProxiedObj::Release()
{
	if (!m_isDead->Get())
	{
		m_obj->Release();
	}
	asAtomicDec(m_refCount);
	if (!m_refCount)
	{
		delete this;
	}
}

void AS_RefCountedObj::AddRef()
{
	asAtomicInc(m_refcount);
}

void AS_RefCountedObj::Release()
{
	if (1 == m_refcount && m_weakrefflag)
	{
		m_weakrefflag->Set(true);
	}

	if (!asAtomicDec(m_refcount))
	{
		delete this;
	}
}

asILockableSharedBool* AS_RefCountedObj::GetWeakRefFlag()
{
	if (!m_weakrefflag)
	{
		asAcquireExclusiveLock();
		if (!m_weakrefflag)
		{
			m_weakrefflag = asCreateLockableSharedBool();
		}
		asReleaseExclusiveLock();
	}
	return m_weakrefflag;
}

AS_RefCountedObj::AS_RefCountedObj()
{
	m_refcount = 1;
	m_weakrefflag = 0;
}

AS_RefCountedObj::~AS_RefCountedObj()
{
	if (m_weakrefflag)
	{
		m_weakrefflag->Release();
	}
}

