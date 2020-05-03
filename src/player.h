#ifndef PLAYER_H_
#define PLAYER_H_
#include "as_refcountedobj.h"
#include "client.h"
#include "as_manager.h"

#include <string>
#include <set>
#include <atomic>

class asIScriptObject;
class asILockableSharedBool;
class asIScriptEngine;
class asIScriptModule;
class asITypeInfo;
class asIScriptContext;

struct Actor;

class Player:
		public AS_RefCountedObj
{
public:
	static void SetUserEventObserverType(asITypeInfo *ti)
	{
		sm_observertype = ti;
	}

	static int SetUserEventCallbackMethods(asITypeInfo *ti);

	static asITypeInfo* GetUserEventObserverType()
	{
		return sm_observertype;
	}

	struct Client *m_pClient;
	struct Actor *m_pActor;

	void Send(std::string &str);
	void Disconnect();
	static Player* Factory();

	bool AddUserEventObserver(asIScriptObject *obj);
	bool RemoveUserEventObserver(asIScriptObject *obj);
	void RemoveAllUserEventObservers();


	void NotifyObserversOfOutput(const std::string& output,
			asIScriptContext* ctx);
	void NotifyObserversOfInput(const std::string &input,
			asIScriptContext *ctx);
	void NotifyObserversOfDisconnect(asIScriptContext *ctx);
protected:
	Player(asIScriptObject *obj);
	~Player();

private:
	std::atomic<bool> m_bConnected;
	std::set<asIScriptObject*> m_observers;
	asIScriptObject* m_firstobserver;
	pthread_rwlock_t m_observers_rwlock;

	static asITypeInfo *sm_observertype;
	static asIScriptFunction *sm_observer_output_method;
	static asIScriptFunction *sm_observer_input_method;
	static asIScriptFunction *sm_observer_dc_method;
};

int LoadPlayerScript(asIScriptEngine *engine, asIScriptModule *module);
int RegisterPlayerProxyClass(asIScriptEngine *engine);
asIScriptObject* CreatePlayerProxy(AngelScriptManager *manager,
		struct Client *pClient);
#endif
