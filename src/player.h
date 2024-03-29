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

class PlayerConnection : public AS_RefCountedObj
{
public:
	struct Client *m_pClient;
	struct Actor *m_pActor;

	//This mfunction sends data to the user
	void Send(std::string &str);

	void Disconnect();
	static PlayerConnection* Factory(struct Client *client);

	void SetInputCallback(asIScriptFunction* cb);
	void SetDisconnectCallback(asIScriptFunction* cb);

	asIScriptFunction* GetInputCallback()
	{
		//This callback receives data from the user
		return m_scriptinputcb;
	}

	asIScriptFunction* GetDisconnectCallback()
	{
		return m_scriptdisconnectcb;
	}

protected:
	PlayerConnection();
	~PlayerConnection();

private:

	std::atomic<bool> m_bConnected;

	asIScriptFunction* m_scriptinputcb, *m_scriptdisconnectcb;
};

int RegisterPlayerConnectionClass(asIScriptEngine *engine);

#endif
