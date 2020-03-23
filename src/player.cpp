#include "player.h"
#include <cstring>
#include <string>

#include "utils.h"
#include "uuid.h"
#include "as_api.h"
#include "as_manager.h"
#include "poolalloc.h"
#include "angelscript.h"

static const char* playerscript =
	"shared abstract class PlayerConnection"
	"{"
	"PlayerConnection()"
	"{"
	"@m_obj = Player_t();"
	"}"
	"void Send(string msg){m_obj.Send(msg);}"
	"void Disconnect(){m_obj.Disconnect();}"
	"void QueueCommand(ICommand@+ cmd, uint32 delay_s, uint32 delay_ns){m_obj.QueueCommand(cmd, delay_s, delay_ns);}"
	"Player_t @opImplCast() {return @m_obj;}"
	"private Player_t @m_obj;"
	"}";

Player::Player(asIScriptObject* obj) : AS_RefCountedObj(obj)
{

}

Player::~Player()
{

}

void Player::Send(std::string& str)
{
	dbgprintf("Trying to send: %s\n", str.c_str());
	Client_WriteTo(m_pClient, str.c_str(), str.length());
}

void Player::Disconnect()
{
	Client_Disconnect(m_pClient);
}

void Player::QueueCommand(asIScriptObject* obj, unsigned int delay_s, unsigned int delay_ns)
{
	ASAPI_QueueClientScriptCommand(m_pClient, obj, delay_s, delay_ns);
}

Player* Player::Factory()
{
	asIScriptContext* ctx = asGetActiveContext();
	asIScriptFunction* func = ctx->GetFunction(0);
	if( func->GetObjectType() == 0 || std::string(func->GetObjectType()->GetName()) != "PlayerConnection" )
	{
		ctx->SetException("Invalid attempt to manually instantiate Player_t");
		return 0;
	}

	// Get the this pointer from the calling function so the Player C++
	// class can be linked with the Player script class
	asIScriptObject *obj = reinterpret_cast<asIScriptObject*>(ctx->GetThisPointer(0));
	return new Player(obj);
}

int LoadPlayerScript(asIScriptEngine* engine, asIScriptModule* module)
{
	return module->AddScriptSection("game", playerscript, strlen(playerscript));
}

int RegisterPlayerProxyClass(asIScriptEngine* engine)
{
	if(engine->RegisterObjectType("Player_t", 0, asOBJ_REF) < 0)
		return -1;
	if(engine->RegisterObjectBehaviour("Player_t", asBEHAVE_FACTORY, "Player_t @f()", asFUNCTION(Player::Factory), asCALL_CDECL) < 0)
		return -1;
	if(engine->RegisterObjectBehaviour("Player_t", asBEHAVE_ADDREF, "void f()", asMETHOD(Player, AddRef), asCALL_THISCALL) < 0)
		return -1;
	if(engine->RegisterObjectBehaviour("Player_t", asBEHAVE_RELEASE, "void f()", asMETHOD(Player, Release), asCALL_THISCALL) < 0)
		return -1;
	if(engine->RegisterObjectMethod("Player_t", "void Send(string& in)", asMETHODPR(Player, Send, (std::string&), void), asCALL_THISCALL) <0)
		return -1;
	if(engine->RegisterObjectMethod("Player_t", "void Disconnect()", asMETHOD(Player, Disconnect), asCALL_THISCALL) < 0)
		return -1;
	if(engine->RegisterObjectMethod("Player_t", "void QueueCommand(ICommand@+ cmd, uint32 delay_s, uint32 delay_ns)",
						asMETHOD(Player, QueueCommand), asCALL_THISCALL) < 0)
		return -1;
	return 0;
}

asIScriptObject* CreatePlayerProxy(AngelScriptManager* manager, struct Client* pClient)
{
	asIScriptEngine* pEngine = manager->engine;
	asIScriptModule* pModule = manager->main_module;
	asIScriptObject* obj = reinterpret_cast<asIScriptObject*>(pEngine->CreateScriptObject(
									pModule->GetTypeInfoByName("Player")
									)
		);
	if(obj)
	{
		Player* pPlayer = *((Player**) obj->GetAddressOfProperty(0));
		pPlayer->m_pClient = pClient;
	}
	else
	{
		ServerLog(SERVERLOG_ERROR, "Failed to find script player class.");
	}
	return obj;
}
