#include "player.h"
#include "as_manager.h"
#include <cstring>
#include <string>
#include <angelscript.h>

static const char* playerscript =
	"shared abstract class Player"
	"{"
	"Player()"
	"{"
	"@m_obj = Player_t();"
	"}"
	"void Send(string msg){m_obj.Send(msg);}"
	"Player_t @opImplCast() {return m_obj;}"
	"private Player_t @m_obj;"
	"}";

Player::Player(asIScriptObject* obj)
{
	m_obj = obj;
	m_refCount = 1;
	m_isDead = obj->GetWeakRefFlag();
	m_isDead->AddRef();
}

Player::~Player()
{
	m_isDead->Release();
}

void Player::Send(std::string& str)
{
	Client_WriteTo(m_pClient, str.c_str(), str.length());
}

void Player::AddRef()
{
	++m_refCount;
	if(!m_isDead->Get())
	{
		m_obj->AddRef();
	}
}

void Player::Release()
{
	if(!m_isDead->Get())
	{
		m_obj->Release();
	}
	if(--m_refCount == 0)
	{
		delete this;
	}
}

Player* Player::Factory()
{
	asIScriptContext* ctx = asGetActiveContext();
	asIScriptFunction* func = ctx->GetFunction(0);
	if( func->GetObjectType() == 0 || std::string(func->GetObjectType()->GetName()) != "Player" )
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

int RegisterPlayerProxyClass(asIScriptEngine* engine, asIScriptModule* module)
{
	int result = 0;
	engine->RegisterObjectType("Player_t", 0, asOBJ_REF);
	engine->RegisterObjectBehaviour("Player_t", asBEHAVE_FACTORY, "Player_t @f()", asFUNCTION(Player::Factory), asCALL_CDECL);
	engine->RegisterObjectBehaviour("Player_t", asBEHAVE_ADDREF, "void f()", asMETHOD(Player, AddRef), asCALL_THISCALL);
	engine->RegisterObjectBehaviour("Player_t", asBEHAVE_RELEASE, "void f()", asMETHOD(Player, Release), asCALL_THISCALL);
	result = engine->RegisterObjectMethod("Player_t", "void Send(string& in)", asMETHODPR(Player, Send, (std::string&), void), asCALL_THISCALL);
	if(result < 0)
	{
		return -1;
	}
//	engine->RegisterObjectProperty("Player_t", "int m_value", asOFFSET(Player, m_value));


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
	Player* pPlayer = *((Player**) obj->GetAddressOfProperty(0));
	pPlayer->m_pClient = pClient;

	return obj;
}
