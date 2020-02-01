#include "as_manager.h"
extern "C"
{
#include "server.h"
#include "serverconfig.h"
#include "talloc.h"
}
#include "as_api.h"
#include "player.h"
#include "scriptstdstring.h"
#include "scriptarray.h"
#include "scripthelper.h"
#include "./angelscriptsdk/sdk/angelscript/include/angelscript.h"
#include "./angelscriptsdk/sdk/angelscript/source/as_jit.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <functional>
#include <pthread.h>
#include <memory>

#define RETURNFAIL_IF(x) if(x){return -1;}

extern "C"
{
	void as_MessageCallback(const asSMessageInfo* msg, void* param)
	{
		const char *type = "ERR ";
		if( msg->type == asMSGTYPE_WARNING )
			type = "WARN";
		else if( msg->type == asMSGTYPE_INFORMATION )
			type = "INFO";

		ServerLog(SERVERLOG_ERROR, "%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, type, msg->message);
	}

	int AngelScriptManager_InitEngine(AngelScriptManager* manager)
	{
		MemoryPool_Init(&manager->mem_pool);
		manager->engine = asCreateScriptEngine();
		#ifdef x86_64
		manager->jit = new asCJITCompiler(0);
		manager->engine->SetEngineProperty(asEP_INCLUDE_JIT_INSTRUCTIONS, 1);
		manager->engine->SetJITCompiler(manager->jit);
		#endif
		RETURNFAIL_IF(!manager->engine);

		manager->engine->SetEngineProperty(asEP_ALLOW_MULTILINE_STRINGS, true);
		RegisterStdString(manager->engine);
		RegisterScriptArray(manager->engine, true);
		RegisterExceptionRoutines(manager->engine);
		manager->engine->SetMessageCallback(asFUNCTION(as_MessageCallback), 0, asCALL_CDECL);
#ifdef DEBUG
		ASContextPool_Init(&manager->ctx_pool, manager->engine, manager, 1);
#else
		ASContextPool_Init(&manager->ctx_pool, manager->engine, manager, 32);
#endif
		return 0;
	}

	static size_t GetFileLength(FILE* fp)
	{
		fseek(fp, 0, SEEK_END);
		size_t len = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		return len;
	}

	int AngelScriptManager_InitAPI(AngelScriptManager* manager, struct Server* server)
	{
		int result = 0;
		asIScriptEngine* pEngine = manager->engine;
		result = RegisterPlayerProxyClass(pEngine, manager->main_module);
		RETURNFAIL_IF(result < 0);

		result = pEngine->RegisterObjectType("Server", 0, asOBJ_REF | asOBJ_NOCOUNT);
		RETURNFAIL_IF(result < 0);

		result = pEngine->RegisterObjectMethod("Server", "void SendToAll(string& in)",
						asFUNCTION(ASAPI_SendToAll), asCALL_CDECL_OBJFIRST);
		RETURNFAIL_IF(result < 0);

		result = pEngine->RegisterInterface("ICommand");
		RETURNFAIL_IF(result < 0);

		result = pEngine->RegisterInterfaceMethod("ICommand", "int opCall()");
		RETURNFAIL_IF(result < 0);

		result = pEngine->RegisterObjectMethod("Server", "void QueueScriptCommand(ICommand@+ cmd, uint32 delay)",
						asFUNCTION(ASAPI_QueueScriptCommand), asCALL_CDECL_OBJFIRST);
		RETURNFAIL_IF(result < 0);

		result = pEngine->RegisterObjectMethod("Server", "void DebugVariables(Player_t@ player)", asFUNCTION(ASAPI_DebugVariables),
						asCALL_CDECL_OBJFIRST);
		RETURNFAIL_IF(result < 0);

		result = pEngine->RegisterGlobalProperty("Server game_server", server);
		RETURNFAIL_IF(result < 0);

		result = pEngine->RegisterGlobalFunction("void Log(string& in)", asFUNCTION(ASAPI_Log), asCALL_CDECL);
		RETURNFAIL_IF(result < 0);


		result = pEngine->RegisterGlobalFunction("void TrimString(const string& in, string& out)", asFUNCTION(ASAPI_TrimString),
							asCALL_CDECL);

		RETURNFAIL_IF(result < 0);


		result = pEngine->RegisterGlobalFunction("void HashPassword(const string& in, string& out)", asFUNCTION(ASAPI_HashPassword),
							asCALL_CDECL);

		RETURNFAIL_IF(result < 0);

		result = pEngine->RegisterGlobalFunction("void GenerateUUID(string& out)", asFUNCTION(ASAPI_GenerateUUID),
							asCALL_CDECL);

		RETURNFAIL_IF(result < 0);


		return 0;
	}

	int AngelScriptManager_LoadServerConfig(AngelScriptManager* manager, struct ServerConfig* serverconfig)
	{
		//Rather than create an ad-hoc domain specific configuration language for the server,
		//let's just use AngelScript since we've already got it.
		//This needs to be called before AngelScriptManager_LoadScripts()
		int result = 0;
		std::string script;
		std::string scriptpath = "./server.cfg";
		FILE* fp = fopen(scriptpath.c_str(), "rb");
		RETURNFAIL_IF(!fp);
		printf("Opened server.cfg\n");

		size_t len = GetFileLength(fp);

		script.resize(len);
		RETURNFAIL_IF(fread(&script[0], len, 1, fp) <= 0);
		fclose(fp);

		manager->config_module = manager->engine->GetModule("config_module", asGM_ALWAYS_CREATE);
		RETURNFAIL_IF(manager->config_module->AddScriptSection("server_configuration", &script[0], len) < 0);

		result = manager->engine->RegisterObjectType("ServerConfig", 0, asOBJ_REF | asOBJ_NOCOUNT);
		RETURNFAIL_IF(result < 0);

		result = manager->engine->RegisterObjectMethod("ServerConfig",
							       "void SetDatabasePathAndFile(string& in, string& in)",
							asFUNCTION(ASAPI_SetDatabasePathAndFile), asCALL_CDECL_OBJFIRST);
		RETURNFAIL_IF(result < 0);
		result = manager->engine->RegisterObjectMethod("ServerConfig", "void SetGameScriptPath(string& in)",
							asFUNCTION(ASAPI_SetGameScriptPath), asCALL_CDECL_OBJFIRST);
		RETURNFAIL_IF(result < 0);
		result = manager->engine->RegisterObjectMethod("ServerConfig", "void SetGameBindAddress(string& in, uint16 port)",
							asFUNCTION(ASAPI_SetGameBindAddress), asCALL_CDECL_OBJFIRST);
		RETURNFAIL_IF(result < 0);
		RETURNFAIL_IF(manager->config_module->Build() < 0);
		manager->server_setup_func = manager->config_module->GetFunctionByDecl("void SetupServer(ServerConfig@ config)");

		RETURNFAIL_IF(0 == manager->server_setup_func);

		size_t ctxidx = ASContextPool_GetFreeContextIndex(&manager->ctx_pool);
		asIScriptContext* ctx = ASContextPool_GetContextAt(&manager->ctx_pool, ctxidx);
		ctx->Prepare(manager->server_setup_func);
		ctx->SetArgObject(0, serverconfig);
		ctx->Execute();
		ASContextPool_ReturnContextByIndex(&manager->ctx_pool, ctxidx);

		return 0;
	}

	int AngelScriptManager_LoadScripts(AngelScriptManager* manager, const char* script_dir)
	{
		//TODO: May want to impose some kind of directory structure on scripts
		std::string script;
		std::string scriptpath = std::string(script_dir) + "test.as";
		FILE* fp = fopen(scriptpath.c_str(), "rb");
		RETURNFAIL_IF(!fp);

		size_t len = GetFileLength(fp);

		script.resize(len);

		RETURNFAIL_IF(fread(&script[0], len, 1, fp) <= 0);

		fclose(fp);

		manager->main_module = manager->engine->GetModule("game_module", asGM_ALWAYS_CREATE);
		RETURNFAIL_IF(manager->main_module->AddScriptSection("game", &script[0], len) < 0);
		RETURNFAIL_IF(LoadPlayerScript(manager->engine, manager->main_module));
		RETURNFAIL_IF(manager->main_module->Build() < 0);

		//TODO: Remove this - debug script class information
		printf("-Class information-\n");
		size_t idx = 0, object_type_count = manager->main_module->GetObjectTypeCount();
		for(; idx < object_type_count; ++idx)
		{
			asITypeInfo* pInfo = manager->main_module->GetObjectTypeByIndex(idx);
			printf("Class %lu: %s\n", idx, pInfo->GetName());
		}

		size_t global_properties = manager->engine->GetGlobalPropertyCount();
		printf("There are %lu global properties.\n", global_properties);
		/////////////////////////////

		#ifdef x86_64
		manager->jit->finalizePages();
		#endif

		manager->world_tick_func = manager->main_module->GetFunctionByDecl("void GameTick()");
		RETURNFAIL_IF(0 == manager->world_tick_func);

		manager->on_player_connect_func = manager->main_module->GetFunctionByDecl("void OnPlayerConnect(Player@ player)");
		RETURNFAIL_IF(0 == manager->on_player_connect_func);

		manager->on_player_disconnect_func = manager->main_module->GetFunctionByDecl("void OnPlayerDisconnect(Player@ player)");
		RETURNFAIL_IF(0 == manager->on_player_disconnect_func);

		manager->on_player_input_func = manager->main_module->GetFunctionByDecl("void OnPlayerInput(Player@ player, string input)");
		RETURNFAIL_IF(0 == manager->on_player_input_func);

		manager->world_tick_scriptcontext = manager->engine->CreateContext();

		return 0;
	}

	void AngelScriptManager_CallOnPlayerDisconnect(AngelScriptManager* manager, struct Client* pClient)
	{
		printf("Calling on player disconnect.\n");
		size_t idx = ASContextPool_GetFreeContextIndex(&manager->ctx_pool);
		asIScriptContext* ctx = ASContextPool_GetContextAt(&manager->ctx_pool, idx);
		ctx->Prepare(manager->on_player_disconnect_func);

		ctx->SetArgObject(0, pClient->player_obj);
		ctx->Execute();

		ASContextPool_ReturnContextByIndex(&manager->ctx_pool, idx);
	}

	void AngelScriptManager_CallOnPlayerInput(AngelScriptManager* manager, struct Client* pClient, const char* input)
	{
		size_t idx = ASContextPool_GetFreeContextIndex(&manager->ctx_pool);
		asIScriptContext* ctx = ASContextPool_GetContextAt(&manager->ctx_pool, idx);
		ctx->Prepare(manager->on_player_input_func);
		ctx->SetArgObject(0, pClient->player_obj);
		std::string strarg(input);
		ctx->SetArgObject(1, &strarg);
		ctx->Execute();

		ASContextPool_ReturnContextByIndex(&manager->ctx_pool, idx);
	}

	void AngelScriptManager_CallOnPlayerConnect(AngelScriptManager* manager, struct Client* pClient)
	{
		size_t idx = ASContextPool_GetFreeContextIndex(&manager->ctx_pool);
		asIScriptContext* ctx = ASContextPool_GetContextAt(&manager->ctx_pool, idx);
		ctx->Prepare(manager->on_player_connect_func);
		asIScriptObject* playerobj = CreatePlayerProxy(manager, pClient);

		pClient->player_obj = playerobj;

		ctx->SetArgObject(0, playerobj);
		ctx->Execute();

		ASContextPool_ReturnContextByIndex(&manager->ctx_pool, idx);
	}

	void AngelScriptManager_RunWorldTick(AngelScriptManager* manager)
	{
		manager->world_tick_scriptcontext->Prepare(manager->world_tick_func);
		manager->world_tick_scriptcontext->Execute();
	}

	void AngelScriptManager_ReleaseEngine(AngelScriptManager* manager)
	{
		manager->world_tick_scriptcontext->Release();
		ASContextPool_Destroy(&manager->ctx_pool);
		manager->engine->Release();

		#if x86_64
		delete manager->jit;
		#endif

		MemoryPool_Destroy(&manager->mem_pool);
	}

	void AngelScriptManager_OnScriptException(AngelScriptManager* manager, asIScriptContext* ctx)
	{
		const char* section_name = 0;
		int col_number = 0;
		int line_number = ctx->GetExceptionLineNumber(&col_number, &section_name);
		asIScriptFunction* func = ctx->GetExceptionFunction();
		ServerLog(SERVERLOG_ERROR, "SCRIPT EXCEPTION '%s' thrown from %s:%s:(%d, %d)!",
			ctx->GetExceptionString(), section_name, func->GetDeclaration(),
			line_number, col_number);
	}

	int ASContextPool_Init(ASContextPool* pPool, asIScriptEngine* pEngine, AngelScriptManager* pManager,
			size_t initial_size)
	{
		pthread_mutex_init(&pPool->mtx, 0);
		pPool->pEngine = pEngine;
		pPool->pContextArray = (asIScriptContext**) talloc(sizeof(asIScriptContext*) * initial_size);
		pPool->pInUseArray = (unsigned char*) talloc(sizeof(unsigned char) * initial_size);

		RETURNFAIL_IF(!pPool->pContextArray || !pPool->pInUseArray);

		memset(pPool->pContextArray, 0, sizeof(asIScriptContext*) * initial_size);
		memset(pPool->pInUseArray, 0, sizeof(unsigned char) * initial_size);


		pPool->ctx_count = initial_size;
		size_t idx = 0;
		for(; idx < initial_size; ++idx)
		{
			asIScriptContext* ctx = pEngine->CreateContext();
			ctx->SetExceptionCallback(asFUNCTION(AngelScriptManager_OnScriptException),
						pManager, asCALL_CDECL_OBJFIRST);

			pPool->pContextArray[idx] = ctx;
		}

		return 0;
	}

	asIScriptContext* ASContextPool_GetContextAt(ASContextPool* pPool, size_t idx)
	{
		if(idx > pPool->ctx_count)
			return 0;

		return pPool->pContextArray[idx];
	}

	void ASContextPool_ReturnContextByIndex(ASContextPool* pPool, size_t idx)
	{
		//Mark context as usable again.
		pPool->pInUseArray[idx] = (unsigned char) 0;
	}

	size_t ASContextPool_GetFreeContextIndex(ASContextPool* pPool)
	{
		pthread_mutex_lock(&pPool->mtx);
		unsigned char* pLoc = (unsigned char*) memchr(pPool->pInUseArray, 0, pPool->ctx_count);
		if(!pLoc)
		{
			size_t oldsize = pPool->ctx_count;
			size_t newsize = oldsize << 1;
			pPool->pContextArray = (asIScriptContext**) trealloc(pPool->pContextArray, newsize * sizeof(asIScriptContext*));
			pPool->pInUseArray = (unsigned char*) trealloc(pPool->pInUseArray, newsize * sizeof(unsigned char));

			memset(&pPool->pInUseArray[oldsize], 0, sizeof(unsigned char) * (newsize - oldsize));
			size_t idx = oldsize;
			for(; idx < newsize; ++idx)
			{
				pPool->pContextArray[idx] = pPool->pEngine->CreateContext();
			}

			pPool->ctx_count = newsize;

			pPool->pInUseArray[oldsize] = (unsigned char) 1;
			pthread_mutex_unlock(&pPool->mtx);
			return oldsize;
		}
		size_t offset = pLoc - pPool->pInUseArray;
		*pLoc = 1;
		pthread_mutex_unlock(&pPool->mtx);
		return offset;
	}

	void ASContextPool_Destroy(ASContextPool* pPool)
	{
		pthread_mutex_lock(&pPool->mtx);
		size_t idx = 0, len = pPool->ctx_count;
		for(; idx < len; ++idx)
		{
			pPool->pContextArray[idx]->Release();
		}
		tfree(pPool->pContextArray);
		tfree(pPool->pInUseArray);
		pthread_mutex_destroy(&pPool->mtx);
	}
}
