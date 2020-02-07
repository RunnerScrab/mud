#include "player.h"
#include "as_manager.h"
#include <cstring>
#include <string>
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
	"protected void SaveProperty(string name, string val){m_obj.SaveProperty(name, val);}"
	"Player_t @opImplCast() {return @m_obj;}"
	"private Player_t @m_obj;"
	"}";

Player::Player(asIScriptObject* obj) : PersistentObj(obj)
{
}

Player::~Player()
{

}

void Player::Send(std::string& str)
{
	printf("Trying to send: %s\n", str.c_str());
	Client_WriteTo(m_pClient, str.c_str(), str.length());
}

void Player::Disconnect()
{
	Client_Disconnect(m_pClient);
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


template<class A, class B>
	B* refCast(A* a)
{
	// If the handle already is a null handle, then just return the null handle
	if( !a ) return 0;

	// Now try to dynamically cast the pointer to the wanted type
	B* b = dynamic_cast<B*>(a);
	if( b != 0 )
	{
		// Since the cast was made, we need to increase the ref counter for the returned handle
		b->AddRef();
	}
	return b;
}

int RegisterPlayerProxyClass(asIScriptEngine* engine, asIScriptModule* module)
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
	if(engine->RegisterObjectMethod("Player_t", "void SaveProperty(string& in, string& in)",
						asMETHODPR(Player, SaveProperty, (const std::string&, const std::string&), void),
						asCALL_THISCALL) < 0)
		return -1;

	if(engine->RegisterObjectMethod("PersistentObj_t", "Player_t@ opCast()", asFUNCTION((refCast<PersistentObj, Player>)), asCALL_CDECL_OBJLAST) < 0)
		return -1;
	if(engine->RegisterObjectMethod("Player_t", "PersistentObj_t@ opImplCast()",
						asFUNCTION((refCast<Player, PersistentObj>)), asCALL_CDECL_OBJLAST) < 0)
		return -1;


	if(engine->RegisterObjectMethod("PersistentObj_t", "const Player_t@ opCast() const",
						asFUNCTION((refCast<PersistentObj, Player>)), asCALL_CDECL_OBJLAST) < 0)
		return -1;
	if(engine->RegisterObjectMethod("Player_t", "const PersistentObj_t@ opImplCast() const",
						asFUNCTION((refCast<Player, PersistentObj>)), asCALL_CDECL_OBJLAST) < 0)
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
		printf("Failed to find script player class.\n");
	}
	return obj;
}
