#include "player.h"
#include <cstring>
#include <string>

#include "utils.h"
#include "uuid.h"
#include "as_api.h"
#include "as_manager.h"
#include "poolalloc.h"
#include "angelscript.h"

asITypeInfo *PlayerConnection::sm_observertype;
asIScriptFunction *PlayerConnection::sm_observer_dc_method;
asIScriptFunction *PlayerConnection::sm_observer_input_method;
asIScriptFunction *PlayerConnection::sm_observer_output_method;

void PlayerConnection::Release()
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

void PlayerConnection::AddRef()
{
	asAtomicInc(m_refcount);
}

asILockableSharedBool* PlayerConnection::GetWeakRefFlag()
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

PlayerConnection::PlayerConnection() :
		m_firstobserver(0)
{
	m_refcount = 1;
	m_weakrefflag = 0;
	pthread_rwlock_init(&m_observers_rwlock, 0);
	m_bConnected = true;
}

PlayerConnection::~PlayerConnection()
{
	ServerLog(SERVERLOG_DEBUG, "PlayerConnection being destroyed.");
	pthread_rwlock_destroy(&m_observers_rwlock);
	if (m_weakrefflag)
	{
		m_weakrefflag->Release();
	}
}

void PlayerConnection::Send(std::string &str)
{
	if (m_bConnected)
	{
		dbgprintf("Trying to send: %s\n", str.c_str());
		Client_WriteTo(m_pClient, str.c_str(), str.length());
		asIScriptContext *ctx = asGetActiveContext();
		if (ctx)
		{
			ctx->PushState();
			NotifyObserversOfOutput(str, ctx);
			ctx->PopState();
		}
	}
	else
	{
		dbgprintf("Tried to send to a disconnected player.\n");
	}
}

void PlayerConnection::Disconnect()
{
	if (!GetWeakRefFlag()->Get())
	{
		m_bConnected = false;
		Client_Disconnect(m_pClient);
	}
}

PlayerConnection* PlayerConnection::Factory(struct Client *client)
{
	PlayerConnection *pconn = new PlayerConnection();
	pconn->m_pClient = client;
	return pconn;
}

int PlayerConnection::SetUserEventCallbackMethods(asITypeInfo *ti)
{
	sm_observer_input_method = ti->GetMethodByName("OnInputReceived");
	sm_observer_dc_method = ti->GetMethodByName("OnDisconnect");
	sm_observer_output_method = ti->GetMethodByName("OnOutputReceived");
	return sm_observer_input_method && sm_observer_dc_method
			&& sm_observer_output_method ? 0 : -1;
}

int RegisterPlayerConnectionClass(asIScriptEngine *engine)
{
	if (engine->RegisterObjectType("PlayerConnection", 0, asOBJ_REF) < 0)
		return -1;

	if (engine->RegisterObjectBehaviour("PlayerConnection", asBEHAVE_ADDREF,
			"void f()", asMETHOD(PlayerConnection, AddRef), asCALL_THISCALL)
			< 0)
		return -1;
	if (engine->RegisterObjectBehaviour("PlayerConnection", asBEHAVE_RELEASE,
			"void f()", asMETHOD(PlayerConnection, Release), asCALL_THISCALL)
			< 0)
		return -1;

	if (engine->RegisterObjectBehaviour("PlayerConnection",
			asBEHAVE_GET_WEAKREF_FLAG, "int& f()",
			asMETHOD(PlayerConnection, GetWeakRefFlag), asCALL_THISCALL) < 0)
		return -1;

	if (engine->RegisterObjectMethod("PlayerConnection",
			"void Send(string& in)",
			asMETHODPR(PlayerConnection, Send, (std::string&), void),
			asCALL_THISCALL) < 0)
		return -1;
	if (engine->RegisterObjectMethod("PlayerConnection", "void Disconnect()",
			asMETHOD(PlayerConnection, Disconnect), asCALL_THISCALL) < 0)
		return -1;

	if (engine->RegisterInterface("IUserEventObserver") < 0)
		return -1;

	if (engine->RegisterInterfaceMethod("IUserEventObserver",
			"void OnOutputReceived(string output)") < 0)
		return -1;
	if (engine->RegisterInterfaceMethod("IUserEventObserver",
			"void OnInputReceived(string input)") < 0)
		return -1;

	if (engine->RegisterInterfaceMethod("IUserEventObserver",
			"void OnDisconnect()") < 0)
		return -1;

	PlayerConnection::SetUserEventObserverType(
			engine->GetTypeInfoByName("IUserEventObserver"));

	if (!PlayerConnection::GetUserEventObserverType())
	{
		ServerLog(SERVERLOG_ERROR,
				"Couldn't get interface type for IUserEventObserver!");
		return -1;
	}

	if (-1
			== PlayerConnection::SetUserEventCallbackMethods(
					PlayerConnection::GetUserEventObserverType()))
	{
		ServerLog(SERVERLOG_ERROR,
				"Failed to set IUserEventObserver callback methods!");
		return -1;
	}

	if (engine->RegisterObjectMethod("PlayerConnection",
			"bool AttachUserEventObserver(IUserEventObserver@ observer)",
			asMETHODPR(PlayerConnection, AddUserEventObserver,
					(asIScriptObject*), bool), asCALL_THISCALL) < 0)
	{
		ServerLog(SERVERLOG_ERROR,
				"Failed to register attach user input event method.");
		return -1;
	}

	if (engine->RegisterObjectMethod("PlayerConnection",
			"bool DetachUserEventObserver(IUserEventObserver@ observer)",
			asMETHODPR(PlayerConnection, RemoveUserEventObserver,
					(asIScriptObject*), bool), asCALL_THISCALL) < 0)
	{
		ServerLog(SERVERLOG_ERROR,
				"Failed to register attach user dc event method.");
		return -1;
	}

	if (engine->RegisterObjectMethod("PlayerConnection",
			"void RemoveAllUserEventObservers()",
			asMETHODPR(PlayerConnection, RemoveAllUserEventObservers, (void),
					void), asCALL_THISCALL) < 0)
	{
		ServerLog(SERVERLOG_ERROR,
				"Failed to register attach user dc event method.");
		return -1;
	}
	return 0;
}

bool PlayerConnection::AddUserEventObserver(asIScriptObject *observer)
{
	if (observer->GetObjectType()->Implements(sm_observertype))
	{
		if (!m_firstobserver)
			m_firstobserver = observer;
		ServerLog(SERVERLOG_STATUS, "Adding observer to set.");
		pthread_rwlock_wrlock(&m_observers_rwlock);
		m_observers.insert(observer);
		pthread_rwlock_unlock(&m_observers_rwlock);
		ServerLog(SERVERLOG_STATUS, "Observer Refcount now %d.",
				observer->GetRefCount());
		observer->Release();
		return true;
	}
	else
	{
		observer->Release();
		return false;
	}
}

bool PlayerConnection::RemoveUserEventObserver(asIScriptObject *observer)
{
	if (observer->GetObjectType()->Implements(sm_observertype))
	{
		ServerLog(SERVERLOG_STATUS,
				"Attempting to acquire lock for Removing observer from set.");
		pthread_rwlock_wrlock(&m_observers_rwlock);
		dbgprintf("Locked observer in %s\n", __FUNCTION__);
		ServerLog(SERVERLOG_STATUS, "Removing observer from set.");
		m_observers.erase(m_observers.find(observer));
		observer->GetWeakRefFlag()->Release();
		pthread_rwlock_unlock(&m_observers_rwlock);
		dbgprintf("Unlocked observer in %s\n", __FUNCTION__);
		ServerLog(SERVERLOG_STATUS, "Done removing observer from set.");
		return true;
	}
	observer->Release();
	return false;
}

void PlayerConnection::RemoveAllUserEventObservers()
{
	//Removes all observers listening to the events for this player
	ServerLog(SERVERLOG_STATUS, "Removing all observers from set.");
	pthread_rwlock_wrlock(&m_observers_rwlock);
	dbgprintf("Locked observer in %s\n", __FUNCTION__);
	for (asIScriptObject *obj : m_observers)
	{
		if (obj->GetWeakRefFlag() && !obj->GetWeakRefFlag()->Get())
		{
			obj->Release();
		}
	}
	m_observers.clear();
	pthread_rwlock_unlock(&m_observers_rwlock);
	dbgprintf("Unlocked observer in %s\n", __FUNCTION__);
	ServerLog(SERVERLOG_STATUS, "Done removing all observers from set.");
}

void PlayerConnection::NotifyObserversOfOutput(const std::string &output,
		asIScriptContext *ctx)
{
	//Caller manages context allocation and release
	std::string strarg = output;
	pthread_rwlock_rdlock(&m_observers_rwlock);
	dbgprintf("Locked observer in %s\n", __FUNCTION__);
	for (std::set<asIScriptObject*>::iterator it = m_observers.begin();
			it != m_observers.end();)
	{
		asIScriptObject *obj = *it;
		asILockableSharedBool *isdead = obj->GetWeakRefFlag();
		if (m_firstobserver == obj)
		{
			ServerLog(SERVERLOG_STATUS, "Sender == obj");
			++it;
		}
		else if (!isdead->Get())
		{
			ServerLog(SERVERLOG_STATUS, "Trying to notify ref with count %d.",
					obj->GetRefCount());
			ctx->Prepare(sm_observer_output_method);
			ctx->SetObject(obj);
			ctx->SetArgObject(0, &strarg);
			ctx->Execute();
			++it;
		}
	}
	pthread_rwlock_unlock(&m_observers_rwlock);
	/*
	 if(isdead->Get())
	 {
	 ServerLog(SERVERLOG_STATUS,
	 "Erasing the last reference to an obj.");
	 obj->Release();
	 auto erasethis = it;
	 ++it;
	 m_observers.erase(erasethis);
	 continue;
	 }
	 */
	dbgprintf("Unlocked observer in %s\n", __FUNCTION__);
}

void PlayerConnection::NotifyObserversOfInput(const std::string &input,
		asIScriptContext *ctx)
{
	//Caller manages context allocation and release
	std::string strarg = input;
	dbgprintf("Trying to lock observer in %s\n", __FUNCTION__);
	pthread_rwlock_rdlock(&m_observers_rwlock);
	dbgprintf("Locked observer in %s\n", __FUNCTION__);
	for (std::set<asIScriptObject*>::iterator it = m_observers.begin();
			it != m_observers.end();)
	{
		asIScriptObject *obj = *it;
		asILockableSharedBool *isdead = obj->GetWeakRefFlag();
		if (!isdead->Get())
		{
			ServerLog(SERVERLOG_STATUS, "Trying to notify ref with count %d.",
					obj->GetRefCount());
			ctx->Prepare(sm_observer_input_method);
			ctx->SetObject(obj);
			ctx->SetArgObject(0, &strarg);
			ctx->Execute();
		}
		++it;

	}
	pthread_rwlock_unlock(&m_observers_rwlock);
	dbgprintf("Unlocked observer in %s\n", __FUNCTION__);
}

void PlayerConnection::NotifyObserversOfDisconnect(asIScriptContext *ctx)
{
	//Caller manages context allocation and release
	pthread_rwlock_rdlock(&m_observers_rwlock);
	dbgprintf("Locked observer in %s\n", __FUNCTION__);
	for (auto it = m_observers.begin(); it != m_observers.end();)
	{
		asIScriptObject *obj = *it;
		if (!obj->GetWeakRefFlag()->Get())
		{
			ctx->Prepare(sm_observer_dc_method);
			ctx->SetObject(obj);
			ctx->Execute();
		}
		++it;
	}

	pthread_rwlock_unlock(&m_observers_rwlock);
	dbgprintf("Unlocked observer in %s\n", __FUNCTION__);
}
