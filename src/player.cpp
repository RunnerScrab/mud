#include "player.h"
#include <cstring>
#include <string>

#include "utils.h"
#include "uuid.h"
#include "as_api.h"
#include "as_manager.h"
#include "poolalloc.h"
#include "angelscript.h"

PlayerConnection::PlayerConnection() :
		m_scriptinputcb(0), m_scriptdisconnectcb(0)
{
	m_bConnected = true;
}

PlayerConnection::~PlayerConnection()
{
	ServerLog(SERVERLOG_DEBUG, "PlayerConnection being destroyed.");

	if (m_scriptinputcb)
	{
		m_scriptinputcb->Release();
		m_scriptinputcb = 0;
	}

	if(m_scriptdisconnectcb)
	{
		m_scriptdisconnectcb->Release();
		m_scriptdisconnectcb = 0;
	}
}

void PlayerConnection::Send(std::string &str)
{
	if (m_bConnected)
	{
		dbgprintf("Trying to send: %s\n", str.c_str());
		Client_WriteTo(m_pClient, str.c_str(), str.length());
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

void PlayerConnection::SetInputCallback(asIScriptFunction *cb)
{
	m_scriptinputcb = cb;
}

void PlayerConnection::SetDisconnectCallback(asIScriptFunction* cb)
{
	m_scriptdisconnectcb = cb;
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

	if (engine->RegisterFuncdef("void InputCallback(string msg)") < 0)
	{
		ServerLog(SERVERLOG_ERROR, "Failed to register callback definition.");
		return -1;
	}

	if (engine->RegisterFuncdef("void DisconnectCallback()") < 0)
	{
		ServerLog(SERVERLOG_ERROR,
				"Failed to register disconnect callback definition.");
		return -1;
	}

	if (engine->RegisterObjectMethod("PlayerConnection",
			"void SetInputCallback(InputCallback@ cb)",
			asMETHODPR(PlayerConnection, SetInputCallback, (asIScriptFunction*),
					void), asCALL_THISCALL) < 0)
	{
		ServerLog(SERVERLOG_ERROR,
				"Failed to register callback setting function.");
		return -1;
	}

	if (engine->RegisterObjectMethod("PlayerConnection",
			"void SetDisconnectCallback(DisconnectCallback@ dc)",
			asMETHODPR(PlayerConnection, SetDisconnectCallback,
					(asIScriptFunction*), void), asCALL_THISCALL) < 0)
	{
		ServerLog(SERVERLOG_ERROR, "Failed to register disconnect callback.");
		return -1;
	}

	return 0;
}
