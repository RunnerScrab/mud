#include "as_api.h"
extern "C"
{
#include "poolalloc.h"
#include "crypto.h"
#include "charvector.h"
#include "uuid.h"
#include "serverconfig.h"
}
#include "player.h"
#include "angelscript.h"
#include <ctype.h>

void ASAPI_SendToAll(struct Server* server, std::string& message)
{
	Server_SendAllClients(server, message.c_str());
}

struct RunScriptCmdPkg
{
	asITypeInfo* cmdtype;
	asIScriptObject* cmd;
	size_t context_handle;
	asIScriptEngine* engine;
	ASContextPool* context_pool;
	MemoryPool* pMemPool; //Memory pool space for this struct was allocated on
};

void* ASAPI_RunScriptCommand(void* pArgs)
{
	struct RunScriptCmdPkg* pPkg = (struct RunScriptCmdPkg*) pArgs;
	asIScriptFunction* func = pPkg->cmdtype->GetMethodByDecl("int opCall()");
	ASContextPool* pctx_pool = pPkg->context_pool;

	//The faster alternative to a context pool would be giving the
	//threadpool workers each their own context they would reuse
	//for every task they ran which required it. This would
	//require differentiating between a script task and an
	//internally generated task everywhere tasks are handled,
	//however, resulting in a slightly less flexible design.

	asIScriptContext* context = ASContextPool_GetContextAt(pctx_pool, pPkg->context_handle);
	context->Prepare(func);
	context->SetObject(pPkg->cmd);
	context->Execute();
	ASContextPool_ReturnContextByIndex(pctx_pool, pPkg->context_handle);
	printf("Running script function\n");
	MemoryPool_Free(pPkg->pMemPool, sizeof(struct RunScriptCmdPkg), pArgs);

	return (void*) 0;
}

const char* GetASTypeName(int type)
{
	static const char* asTypeNames[] =
		{
			"void", "bool", "int8", "int16", "int32", "int64",
			"uint8", "uint16", "uint32", "uint64", "float",
			"double", "objhandle", "handletoconst", "object",
			"appobject", "scriptobject", "template", "seqnumber"
		};

	switch(type)
	{
	case asTYPEID_VOID:
		return asTypeNames[0];
	case asTYPEID_BOOL:
		return asTypeNames[1];
	case asTYPEID_INT8:
		return asTypeNames[2];
	case asTYPEID_INT16:
		return asTypeNames[3];
	case asTYPEID_INT32:
		return asTypeNames[4];
	case asTYPEID_INT64:
		return asTypeNames[5];
	case asTYPEID_UINT8:
		return asTypeNames[6];
	case asTYPEID_UINT16:
		return asTypeNames[7];
	case asTYPEID_UINT32:
		return asTypeNames[8];
	case asTYPEID_UINT64:
		return asTypeNames[9];
	case asTYPEID_FLOAT:
		return asTypeNames[10];
	case asTYPEID_DOUBLE:
		return asTypeNames[11];
	default:
		if(asTYPEID_OBJHANDLE & type)
			return asTypeNames[12];
		if(asTYPEID_HANDLETOCONST & type)
			return asTypeNames[13];
		if(asTYPEID_MASK_OBJECT & type)
			return asTypeNames[14];
		if(asTYPEID_APPOBJECT & type)
			return asTypeNames[15];
		if(asTYPEID_SCRIPTOBJECT & type)
			return asTypeNames[16];
		if(asTYPEID_TEMPLATE & type)
			return asTypeNames[17];
		if(asTYPEID_MASK_SEQNBR & type)
			return asTypeNames[18];

		return "unknown";
	}
}

std::string ASAPI_DebugObject(asIScriptObject* obj)
{
	asITypeInfo* typeinfo = obj->GetObjectType();
	return typeinfo->GetName();
}

void ASAPI_DebugVariables(struct Server* server, Player* playerobj)
{
	if(playerobj)
	{
		AngelScriptManager* manager = &server->as_manager;
		asIScriptModule* module = manager->main_module;
		size_t gpropertycount = manager->engine->GetGlobalPropertyCount();
		size_t gvarcount = module->GetGlobalVarCount();
		Client_Sendf(playerobj->m_pClient,
			"Debug Variables called.\r\n"
			"Global Properties: %lu\r\n"
			"Global Functions: %lu\r\n"
			"Global Variables: %lu\r\n",
			gpropertycount, module->GetFunctionCount(), gvarcount);
		size_t idx = 0;
		for(; idx < gvarcount; ++idx)
		{
			const char* name = 0;
			int vartype = 0;
			module->GetGlobalVar(idx, &name, 0, &vartype, 0);
			Client_Sendf(playerobj->m_pClient, "Var %lu: %s <%s>\r\n",
				idx, name, GetASTypeName(vartype));
			if(vartype & asTYPEID_OBJHANDLE)
			{

			}
			else if(vartype & asTYPEID_SCRIPTOBJECT)
			{
				void* p = module->GetAddressOfGlobalVar(idx);
				asIScriptObject* obj = reinterpret_cast<asIScriptObject*>(p);
				asITypeInfo* typeinfo = obj->GetObjectType();
				if(typeinfo)
				{
					Client_Sendf(playerobj->m_pClient, "Was able to retrieve type info.\r\n");
				}

			}
			else if(vartype & asTYPEID_APPOBJECT)
			{

			}
		}
		playerobj->Release();
	}
}

void ASAPI_QueueScriptCommand(struct Server* server, asIScriptObject* obj, unsigned int delay)
{
	printf("Attempting to queue script command.\n");
	if(obj)
	{
		printf("Queueing script command.\n");
		struct RunScriptCmdPkg* pkg = (struct RunScriptCmdPkg*) MemoryPool_Alloc(
			&server->as_manager.mem_pool, sizeof(struct RunScriptCmdPkg));
		pkg->cmd = obj;
		pkg->cmdtype = server->as_manager.main_module->GetTypeInfoByDecl("ICommand");
		pkg->pMemPool = &server->as_manager.mem_pool;
		pkg->engine = server->as_manager.engine;
		size_t hfreectx = ASContextPool_GetFreeContextIndex(&server->as_manager.ctx_pool);
		pkg->context_pool = &server->as_manager.ctx_pool;
		pkg->context_handle = hfreectx;
		Server_AddTimedTask(server, ASAPI_RunScriptCommand,
				time(0) + delay, pkg, 0);
	}
}

void ASAPI_Log(std::string& message)
{
	ServerLog(SERVERLOG_STATUS, message.c_str());
}

void ASAPI_TrimString(const std::string& in, std::string& out)
{
	size_t idx = in.length() - 1;
	for(; idx > 0 && isspace(in[idx]); --idx);
	out = in.substr(0, idx + 1);
}

void ASAPI_HashPassword(const std::string& password, std::string& out)
{
	cv_t buf;
	cv_init(&buf, 256);
	printf("Received '%s' to hash.\n", password.c_str());
	if(CryptoManager_HashPassword(password.c_str(), password.length(), &buf) >= 0)
	{
		printf("Hash successful. Result: '%s'\n", buf.data);
		out.assign(buf.data);
	}
	else
	{
		printf("Hashing FAILED!\n");
	}
	cv_destroy(&buf);
}

void ASAPI_GenerateUUID(std::string& out)
{
	cv_t buf;
	union UUID uuid;
	cv_init(&buf, 38);
	GenerateUUID(&uuid);
	UUIDToString(&uuid, &buf);
	out.assign(buf.data);
	cv_destroy(&buf);
}

void ASAPI_SetDatabasePathAndFile(struct ServerConfig* config, std::string& path, std::string& filename)
{
	std::string fullpath = path + filename;
	strncpy(config->dbfilepath, fullpath.c_str(), sizeof(char) * fullpath.length());
	ServerLog(SERVERLOG_STATUS, "Database filepath set to %s.", config->dbfilepath);
}

void ASAPI_SetGameScriptPath(struct ServerConfig* config, std::string& path)
{
	strncpy(config->scriptpath, path.c_str(),
		sizeof(char) * 255);
	ServerLog(SERVERLOG_STATUS, "Game script path set to %s\n", config->scriptpath);
}

void ASAPI_SetGameBindAddress(struct ServerConfig* config, std::string& addr, unsigned short port)
{
	strncpy(config->bindip, addr.c_str(), addr.length());
	config->bindport = port;
}
