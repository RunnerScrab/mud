#ifndef PLAYER_H_
#define PLAYER_H_
#include "as_refcountedobj.h"
#include "persistentobj.h"
#include "client.h"
#include "as_manager.h"


#include <string>

class asIScriptObject;
class asILockableSharedBool;
class asIScriptEngine;
class asIScriptModule;

class Player : public PersistentObj
{
public:
	struct Client* m_pClient;
	void QueueCommand(asIScriptObject* obj, unsigned int delay_s, unsigned int delay_ns);
	void Send(std::string& str);
	void Disconnect();
	static Player* Factory();
protected:
	Player(asIScriptObject* obj);
	~Player();
};

int LoadPlayerScript(asIScriptEngine* engine, asIScriptModule* module);
int RegisterPlayerProxyClass(asIScriptEngine* engine, asIScriptModule* module);
asIScriptObject* CreatePlayerProxy(AngelScriptManager* manager, struct Client* pClient);
#endif
