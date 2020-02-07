#include "as_refcountedobj.h"
#include "as_manager.h"
#include <cstring>
#include <string>
#include "angelscript.h"

AS_RefCountedObj::AS_RefCountedObj(asIScriptObject* obj)
{
	m_obj = obj;
	m_refCount = 1;
	m_isDead = obj->GetWeakRefFlag();
	m_isDead->AddRef();
}

AS_RefCountedObj::~AS_RefCountedObj()
{
	m_isDead->Release();
}

void AS_RefCountedObj::AddRef()
{
	asAtomicInc(m_refCount);
	if(!m_isDead->Get())
	{
		m_obj->AddRef();
	}
}

void AS_RefCountedObj::Release()
{
	if(!m_isDead->Get())
	{
		m_obj->Release();
	}
	asAtomicDec(m_refCount);
	if(!m_refCount)
	{
		delete this;
	}
}
