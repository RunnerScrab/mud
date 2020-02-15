#include "player.h"
#include <cstring>
#include <string>

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
	"protected void SaveProperty(string name, string val){m_obj.SaveProperty(name, val);}"
	"protected void SaveProperty(string name, uint8 val){m_obj.SaveProperty(name, val);}"
	"protected void SaveProperty(string name, uint16 val){m_obj.SaveProperty(name, val);}"
	"protected void SaveProperty(string name, uint val){m_obj.SaveProperty(name, val);}"
	"protected void SaveProperty(string name, uint64 val){m_obj.SaveProperty(name, val);}"
	"protected void SaveProperty(string name, int8 val){m_obj.SaveProperty(name, val);}"
	"protected void SaveProperty(string name, int16 val){m_obj.SaveProperty(name, val);}"
	"protected void SaveProperty(string name, int val){m_obj.SaveProperty(name, val);}"
	"protected void SaveProperty(string name, int64 val){m_obj.SaveProperty(name, val);}"
	"protected void SaveProperty(string name, float val){m_obj.SaveProperty(name, val);}"
	"protected void SaveProperty(string name, double val){m_obj.SaveProperty(name, val);}"
	"protected void SaveProperty(string name, uuid val){m_obj.SaveProperty(name, val);}"
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
	if(engine->RegisterObjectMethod("Player_t", "void QueueCommand(ICommand@+ cmd, uint32 delay_s, uint32 delay_ns)",
						asMETHOD(Player, QueueCommand), asCALL_THISCALL) < 0)
		return -1;

	if(engine->RegisterObjectMethod("Player_t", "void SaveProperty(string& in, string& in)",
						asMETHODPR(Player, SaveProperty, (const std::string&, const std::string&), void),
						asCALL_THISCALL) < 0)
		return -1;
	if(engine->RegisterObjectMethod("Player_t", "void SaveProperty(string& in, uint8 v)",
						asMETHODPR(Player, SavePropertyUINT8, (const std::string&, unsigned char), void),
						asCALL_THISCALL) < 0)
		return -1;
	if(engine->RegisterObjectMethod("Player_t", "void SaveProperty(string& in, uint16 v)",
						asMETHODPR(Player, SavePropertyUINT16, (const std::string&, unsigned short), void),
						asCALL_THISCALL) < 0)
		return -1;
	if(engine->RegisterObjectMethod("Player_t", "void SaveProperty(string& in, uint v)",
						asMETHODPR(Player, SavePropertyUINT32, (const std::string&, unsigned int), void),
						asCALL_THISCALL) < 0)
		return -1;
	if(engine->RegisterObjectMethod("Player_t", "void SaveProperty(string& in, uint64 v)",
						asMETHODPR(Player, SavePropertyUINT64, (const std::string&, unsigned long long), void),
						asCALL_THISCALL) < 0)
		return -1;

	if(engine->RegisterObjectMethod("Player_t", "void SaveProperty(string& in, int8 v)",
						asMETHODPR(Player, SavePropertyINT8, (const std::string&, char), void),
						asCALL_THISCALL) < 0)
		return -1;
	if(engine->RegisterObjectMethod("Player_t", "void SaveProperty(string& in, int16 v)",
						asMETHODPR(Player, SavePropertyINT16, (const std::string&, short), void),
						asCALL_THISCALL) < 0)
		return -1;
	if(engine->RegisterObjectMethod("Player_t", "void SaveProperty(string& in, int v)",
						asMETHODPR(Player, SavePropertyINT32, (const std::string&, int), void),
						asCALL_THISCALL) < 0)
		return -1;

	if(engine->RegisterObjectMethod("Player_t", "void SaveProperty(string& in, int64 v)",
						asMETHODPR(Player, SavePropertyINT64, (const std::string&, long long), void),
						asCALL_THISCALL) < 0)
		return -1;

	if(engine->RegisterObjectMethod("Player_t", "void SaveProperty(string& in, float v)",
						asMETHODPR(Player, SavePropertyFloat, (const std::string&, float), void),
						asCALL_THISCALL) < 0)
		return -1;

	if(engine->RegisterObjectMethod("Player_t", "void SaveProperty(string& in, double v)",
						asMETHODPR(Player, SavePropertyDouble, (const std::string&, double), void),
						asCALL_THISCALL) < 0)
		return -1;

	if(engine->RegisterObjectMethod("Player_t", "void SaveProperty(string& in, uuid& in)",
						asMETHODPR(Player, SavePropertyUUID, (const std::string&, const UUID&), void),
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
