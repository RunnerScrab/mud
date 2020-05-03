#include "player.h"
#include <cstring>
#include <string>

#include "utils.h"
#include "uuid.h"
#include "as_api.h"
#include "as_manager.h"
#include "poolalloc.h"
#include "angelscript.h"

static const char *playerscript = "shared abstract class PlayerConnection"
		"{"
		"PlayerConnection()"
		"{"
		"@m_obj = Player_t();"
		"}"
		"void Send(string msg){m_obj.Send(msg);}"
		"void Disconnect(){m_obj.Disconnect();}"
		"Player_t @opImplCast() {return @m_obj;}"
		"bool AttachUserEventObserver(IUserEventObserver@ observer)"
		"{ return m_obj.AttachUserEventObserver(observer); }"
		"bool DetachUserEventObserver(IUserEventObserver@ observer)"
		"{ return m_obj.RemoveUserEventObserver(observer); }"
		"void RemoveAllUserEventObservers()"
		"{ m_obj.RemoveAllUserEventObservers();}"
		"private Player_t @m_obj;"
		"}";

asITypeInfo *Player::sm_observertype;
asIScriptFunction *Player::sm_observer_dc_method;
asIScriptFunction *Player::sm_observer_input_method;
asIScriptFunction *Player::sm_observer_output_method;

Player::Player(asIScriptObject *obj) :
		AS_RefCountedObj(obj), m_firstobserver(0)
{
	pthread_rwlock_init(&m_observers_rwlock, 0);
	m_bConnected = true;
}

Player::~Player()
{
	ServerLog(SERVERLOG_DEBUG, "Player being destroyed.");
	pthread_rwlock_destroy(&m_observers_rwlock);
}

void Player::Send(std::string &str)
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

void Player::Disconnect()
{
	if (!GetWeakRefFlag()->Get())
	{
		m_bConnected = false;
		Client_Disconnect(m_pClient);
	}
}

Player* Player::Factory()
{
	asIScriptContext *ctx = asGetActiveContext();
	asIScriptFunction *func = ctx->GetFunction(0);
	if (func->GetObjectType() == 0
			|| std::string(func->GetObjectType()->GetName())
					!= "PlayerConnection")
	{
		ctx->SetException("Invalid attempt to manually instantiate Player_t");
		return 0;
	}

	// Get the this pointer from the calling function so the Player C++
	// class can be linked with the Player script class
	asIScriptObject *obj =
			reinterpret_cast<asIScriptObject*>(ctx->GetThisPointer(0));
	return new Player(obj);
}

int Player::SetUserEventCallbackMethods(asITypeInfo *ti)
{
	sm_observer_input_method = ti->GetMethodByName("OnInputReceived");
	sm_observer_dc_method = ti->GetMethodByName("OnDisconnect");
	sm_observer_output_method = ti->GetMethodByName("OnOutputReceived");
	return sm_observer_input_method && sm_observer_dc_method
			&& sm_observer_output_method ? 0 : -1;
}

int LoadPlayerScript(asIScriptEngine *engine, asIScriptModule *module)
{
	return module->AddScriptSection("game", playerscript, strlen(playerscript));
}

int RegisterPlayerProxyClass(asIScriptEngine *engine)
{
	if (engine->RegisterObjectType("Player_t", 0, asOBJ_REF) < 0)
		return -1;
	if (engine->RegisterObjectBehaviour("Player_t", asBEHAVE_FACTORY,
			"Player_t @f()", asFUNCTION(Player::Factory), asCALL_CDECL) < 0)
		return -1;
	if (engine->RegisterObjectBehaviour("Player_t", asBEHAVE_ADDREF, "void f()",
			asMETHOD(Player, AddRef), asCALL_THISCALL) < 0)
		return -1;
	if (engine->RegisterObjectBehaviour("Player_t", asBEHAVE_RELEASE,
			"void f()", asMETHOD(Player, Release), asCALL_THISCALL) < 0)
		return -1;
	if (engine->RegisterObjectMethod("Player_t", "void Send(string& in)",
			asMETHODPR(Player, Send, (std::string&), void), asCALL_THISCALL)
			< 0)
		return -1;
	if (engine->RegisterObjectMethod("Player_t", "void Disconnect()",
			asMETHOD(Player, Disconnect), asCALL_THISCALL) < 0)
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

	Player::SetUserEventObserverType(
			engine->GetTypeInfoByName("IUserEventObserver"));

	if (!Player::GetUserEventObserverType())
	{
		ServerLog(SERVERLOG_ERROR,
				"Couldn't get interface type for IUserEventObserver!");
		return -1;
	}

	if (-1
			== Player::SetUserEventCallbackMethods(
					Player::GetUserEventObserverType()))
	{
		ServerLog(SERVERLOG_ERROR,
				"Failed to set IUserEventObserver callback methods!");
		return -1;
	}

	if (engine->RegisterObjectMethod("Player_t",
			"bool AttachUserEventObserver(IUserEventObserver@ observer)",
			asMETHODPR(Player, AddUserEventObserver, (asIScriptObject*), bool),
			asCALL_THISCALL) < 0)
	{
		ServerLog(SERVERLOG_ERROR,
				"Failed to register attach user input event method.");
		return -1;
	}

	if (engine->RegisterObjectMethod("Player_t",
			"bool RemoveUserEventObserver(IUserEventObserver@ observer)",
			asMETHODPR(Player, RemoveUserEventObserver, (asIScriptObject*),
					bool), asCALL_THISCALL) < 0)
	{
		ServerLog(SERVERLOG_ERROR,
				"Failed to register attach user dc event method.");
		return -1;
	}

	if (engine->RegisterObjectMethod("Player_t",
			"void RemoveAllUserEventObservers()",
			asMETHODPR(Player, RemoveAllUserEventObservers, (void), void),
			asCALL_THISCALL) < 0)
	{
		ServerLog(SERVERLOG_ERROR,
				"Failed to register attach user dc event method.");
		return -1;
	}
	return 0;
}

bool Player::AddUserEventObserver(asIScriptObject *observer)
{
	if (observer->GetObjectType()->Implements(sm_observertype))
	{
		if(!m_firstobserver)
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

bool Player::RemoveUserEventObserver(asIScriptObject *observer)
{
	if (observer->GetObjectType()->Implements(sm_observertype))
	{
		ServerLog(SERVERLOG_STATUS,
				"Attempting to acquire lock for Removing observer from set.");
		pthread_rwlock_wrlock(&m_observers_rwlock);
		ServerLog(SERVERLOG_STATUS, "Removing observer from set.");
		m_observers.erase(m_observers.find(observer));
		pthread_rwlock_unlock(&m_observers_rwlock);
		ServerLog(SERVERLOG_STATUS, "Done removing observer from set.");
		return true;
	}
	observer->Release();
	return false;
}

void Player::RemoveAllUserEventObservers()
{
	//Removes all observers listening to the events for this player
	ServerLog(SERVERLOG_STATUS, "Removing all observers from set.");
	pthread_rwlock_wrlock(&m_observers_rwlock);
	for (asIScriptObject *obj : m_observers)
	{
		if (obj->GetWeakRefFlag() && !obj->GetWeakRefFlag()->Get())
		{
			obj->Release();
		}
	}
	m_observers.clear();
	pthread_rwlock_unlock(&m_observers_rwlock);
	ServerLog(SERVERLOG_STATUS, "Done removing all observers from set.");
}

void Player::NotifyObserversOfOutput(const std::string &output,
		asIScriptContext *ctx)
{
	//Caller manages context allocation and release
	std::string strarg = output;
	pthread_rwlock_rdlock(&m_observers_rwlock);
	for (std::set<asIScriptObject*>::iterator it = m_observers.begin();
			it != m_observers.end();)
	{
		asIScriptObject *obj = *it;
		asILockableSharedBool *isdead = obj->GetWeakRefFlag();
		if (m_firstobserver == obj)
		{
			ServerLog(SERVERLOG_STATUS, "Sender == obj");
			++it;
			continue;
		}
		if (!isdead->Get())
		{
			ServerLog(SERVERLOG_STATUS, "Trying to notify ref with count %d.",
					obj->GetRefCount());
			ctx->Prepare(sm_observer_output_method);
			ctx->SetObject(obj);
			ctx->SetArgObject(0, &strarg);
			ctx->Execute();
			++it;
		}
		else
		{
			ServerLog(SERVERLOG_STATUS,
					"Erasing the last reference to an obj.");
			obj->Release();
			auto erasethis = it;
			++it;
			m_observers.erase(erasethis);
		}
	}
	pthread_rwlock_unlock(&m_observers_rwlock);
}

void Player::NotifyObserversOfInput(const std::string &input,
		asIScriptContext *ctx)
{
	//Caller manages context allocation and release
	std::string strarg = input;
	pthread_rwlock_rdlock(&m_observers_rwlock);
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
			++it;
		}
		else
		{
			ServerLog(SERVERLOG_STATUS,
					"Erasing the last reference to an obj.");
			obj->Release();
			auto erasethis = it;
			++it;
			m_observers.erase(erasethis);
		}
	}
	pthread_rwlock_unlock(&m_observers_rwlock);
}

void Player::NotifyObserversOfDisconnect(asIScriptContext *ctx)
{
	//Caller manages context allocation and release
	pthread_rwlock_rdlock(&m_observers_rwlock);
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
}

asIScriptObject* CreatePlayerProxy(AngelScriptManager *manager,
		struct Client *pClient)
{
	asIScriptEngine *pEngine = manager->engine;
	asIScriptModule *pModule = manager->main_module;
	asIScriptObject *obj =
			reinterpret_cast<asIScriptObject*>(pEngine->CreateScriptObject(
					pModule->GetTypeInfoByName("PlayerConnection")));
	if (obj)
	{
		Player *pPlayer = *((Player**) obj->GetAddressOfProperty(0));
		pPlayer->m_pClient = pClient;
	}
	else
	{
		ServerLog(SERVERLOG_ERROR, "Failed to find script player class.");
	}
	return obj;
}
