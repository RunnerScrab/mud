#include "as_api.h"
extern "C"
{
#include "serverconfig.h"
#include "talloc.h"
#include "poolalloc.h"
}

#include "player.h"
#include "scriptstdstring.h"
#include "scriptarray.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <functional>
#include <pthread.h>
#include <memory>

#include "./angelscriptsdk/sdk/angelscript/include/angelscript.h"
#include "./angelscriptsdk/sdk/angelscript/source/as_jit.h"

#define RETURNFAIL_IF(x) if(x){return -1;}

extern "C"
{

	typedef struct
	{
		void** pContextArray;
		unsigned char* pInUseArray;

		size_t ctx_count;

		void* pEngine;

		pthread_mutex_t mtx;
	} ASContextPool;

	typedef struct
	{
		/*
		  asIScriptEngine* engine;
		  asCJITCompiler* jit;

		  asIScriptModule* main_module, *config_module;

		  asIScriptFunction* world_tick_func, *on_player_connect_func;
		  asIScriptFunction* on_player_disconnect_func, *on_player_input_func;
		  asIScriptFunction* server_setup_func;
		  asIScriptContext* world_tick_scriptcontext;
		*/

		void* engine;
		void* jit;
		void* main_module, *config_module;
		void* world_tick_func, *on_player_connect_func;
		void* on_player_disconnect_func, *on_player_input_func;
		void* server_setup_func;
		void* world_tick_scriptcontext;

		struct MemoryPool mem_pool;
		ASContextPool ctx_pool;

		size_t next_free_context_idx;
	}
		AngelScriptManager;


	int AngelScriptManager_InitEngine(AngelScriptManager* manager);
	int AngelScriptManager_LoadServerConfig(AngelScriptManager* manager, struct ServerConfig* servercfg);
	int AngelScriptManager_LoadScripts(AngelScriptManager* manager, const char* script_dir);
	int AngelScriptManager_InitAPI(AngelScriptManager* manager, struct Server* server);
	void AngelScriptManager_RunWorldTick(AngelScriptManager* manager);
	void AngelScriptManager_ReleaseEngine(AngelScriptManager* manager);

	void AngelScriptManager_CallOnPlayerConnect(AngelScriptManager* manager, struct Client* pClient);
	void AngelScriptManager_CallOnPlayerDisconnect(AngelScriptManager* manager, struct Client* pClient);
	void AngelScriptManager_CallOnPlayerInput(AngelScriptManager* manager, struct Client* pClient,
						const char* input);


	int ASContextPool_Init(ASContextPool* pPool, void* pEngine, size_t initial_size);
	void* ASContextPool_GetContextAt(ASContextPool* pPool, size_t idx);
	void ASContextPool_ReturnContextByIndex(ASContextPool* pPool, size_t idx);
	size_t ASContextPool_GetFreeContextIndex(ASContextPool* pPool);
	void ASContextPool_Destroy(ASContextPool* pPool);

	void as_MessageCallback(const asSMessageInfo* msg, void* param)
	{
		const char *type = "ERR ";
		if( msg->type == asMSGTYPE_WARNING )
			type = "WARN";
		else if( msg->type == asMSGTYPE_INFORMATION )
			type = "INFO";


//		ServerLog(SERVERLOG_ERROR, "%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, type, msg->message);
		printf("%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, type, msg->message);
	}

	int AngelScriptManager_InitEngine(AngelScriptManager* manager)
	{
		MemoryPool_Init(&manager->mem_pool);
		manager->engine = asCreateScriptEngine();
		manager->jit = new asCJITCompiler(0);
		asIScriptEngine* engine = reinterpret_cast<asIScriptEngine*>(manager->engine);
		engine->SetEngineProperty(asEP_INCLUDE_JIT_INSTRUCTIONS, 1);
		engine->SetJITCompiler(reinterpret_cast<asIJITCompiler*>(manager->jit));

		engine->SetEngineProperty(asEP_ALLOW_MULTILINE_STRINGS, true);
		RegisterStdString(engine);
		RegisterScriptArray(engine, true);
		engine->SetMessageCallback(asFUNCTION(as_MessageCallback), 0, asCALL_CDECL);
#ifdef DEBUG
		ASContextPool_Init(&manager->ctx_pool, engine, 1);
#else
		ASContextPool_Init(&manager->ctx_pool, engine, 32);
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
		asIScriptEngine* pEngine = reinterpret_cast<asIScriptEngine*>(manager->engine);
		result = RegisterPlayerProxyClass(pEngine, reinterpret_cast<asIScriptModule*>(manager->main_module));
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
		printf("Loaded:\n%s\n", script.c_str());
		asIScriptEngine* engine = reinterpret_cast<asIScriptEngine*>(manager->engine);
		asIScriptModule* config_module = engine->GetModule("config_module", asGM_ALWAYS_CREATE);
		manager->config_module = config_module;

		RETURNFAIL_IF(config_module->AddScriptSection("server_configuration", &script[0], len) < 0);
		printf("Loaded server configuration script\n");

		result = engine->RegisterObjectType("ServerConfig", 0, asOBJ_REF | asOBJ_NOCOUNT);
		RETURNFAIL_IF(result < 0);
		printf("Registered server configuration type.\n");
		result = engine->RegisterObjectMethod("ServerConfig", "void SetDatabasePath(string& in)",
						asFUNCTION(ASAPI_SetDatabasePath), asCALL_CDECL_OBJFIRST);
		RETURNFAIL_IF(result < 0);
		result = engine->RegisterObjectMethod("ServerConfig", "void SetGameScriptPath(string& in)",
						asFUNCTION(ASAPI_SetGameScriptPath), asCALL_CDECL_OBJFIRST);
		RETURNFAIL_IF(result < 0);
		result =engine->RegisterObjectMethod("ServerConfig", "void SetGameBindAddress(string& in, uint16 port)",
						asFUNCTION(ASAPI_SetGameBindAddress), asCALL_CDECL_OBJFIRST);
		RETURNFAIL_IF(result < 0);
		printf("Registered ServerConfig methods.\n");
		RETURNFAIL_IF(config_module->Build() < 0);
		manager->server_setup_func = config_module->GetFunctionByDecl("void SetupServer(ServerConfig@ config)");

		RETURNFAIL_IF(0 == manager->server_setup_func);
		printf("Found setup function.\n");

		size_t ctxidx = ASContextPool_GetFreeContextIndex(&manager->ctx_pool);
		asIScriptContext* ctx = reinterpret_cast<asIScriptContext*>(ASContextPool_GetContextAt(&manager->ctx_pool, ctxidx));
		ctx->Prepare(reinterpret_cast<asIScriptFunction*>(manager->server_setup_func));
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
		asIScriptEngine* engine = reinterpret_cast<asIScriptEngine*>(manager->engine);
		asIScriptModule* main_module = engine->GetModule("game_module", asGM_ALWAYS_CREATE);
		manager->main_module = main_module;
		RETURNFAIL_IF(main_module->AddScriptSection("game", &script[0], len) < 0);
		RETURNFAIL_IF(LoadPlayerScript(engine, main_module));
		RETURNFAIL_IF(main_module->Build() < 0);

		//TODO: Remove this - debug script class information
		printf("-Class information-\n");
		size_t idx = 0, object_type_count = main_module->GetObjectTypeCount();
		for(; idx < object_type_count; ++idx)
		{
			asITypeInfo* pInfo = main_module->GetObjectTypeByIndex(idx);
			printf("Class %lu: %s\n", idx, pInfo->GetName());
		}

		size_t global_properties = engine->GetGlobalPropertyCount();
		printf("There are %lu global properties.\n", global_properties);
		/////////////////////////////

		reinterpret_cast<asCJITCompiler*>(manager->jit)->finalizePages();

		manager->world_tick_func = main_module->GetFunctionByDecl("void GameTick()");
		RETURNFAIL_IF(0 == manager->world_tick_func);

		manager->on_player_connect_func = main_module->GetFunctionByDecl("void OnPlayerConnect(Player@ player)");
		RETURNFAIL_IF(0 == manager->on_player_connect_func);

		manager->on_player_disconnect_func = main_module->GetFunctionByDecl("void OnPlayerDisconnect(Player@ player)");
		RETURNFAIL_IF(0 == manager->on_player_disconnect_func);

		manager->on_player_input_func = main_module->GetFunctionByDecl("void OnPlayerInput(Player@ player, string input)");
		RETURNFAIL_IF(0 == manager->on_player_input_func);

		manager->world_tick_scriptcontext = (void*) engine->CreateContext();
		RETURNFAIL_IF(0 == manager->world_tick_scriptcontext);
		return 0;
	}

	void AngelScriptManager_CallOnPlayerDisconnect(AngelScriptManager* manager, struct Client* pClient)
	{
		printf("Calling on player disconnect.\n");
		size_t idx = ASContextPool_GetFreeContextIndex(&manager->ctx_pool);
		asIScriptContext* ctx = reinterpret_cast<asIScriptContext*>(ASContextPool_GetContextAt(&manager->ctx_pool, idx));
		ctx->Prepare(reinterpret_cast<asIScriptFunction*>(manager->on_player_disconnect_func));

		ctx->SetArgObject(0, pClient->player_obj);
		ctx->Execute();

		ASContextPool_ReturnContextByIndex(&manager->ctx_pool, idx);
	}

	void AngelScriptManager_CallOnPlayerInput(AngelScriptManager* manager, struct Client* pClient, const char* input)
	{
		size_t idx = ASContextPool_GetFreeContextIndex(&manager->ctx_pool);
		asIScriptContext* ctx = reinterpret_cast<asIScriptContext*>(ASContextPool_GetContextAt(&manager->ctx_pool, idx));
		ctx->Prepare(reinterpret_cast<asIScriptFunction*>(manager->on_player_input_func));
		ctx->SetArgObject(0, pClient->player_obj);
		std::string strarg(input);
		ctx->SetArgObject(1, &strarg);
		ctx->Execute();

		ASContextPool_ReturnContextByIndex(&manager->ctx_pool, idx);
	}

	void AngelScriptManager_CallOnPlayerConnect(AngelScriptManager* manager, struct Client* pClient)
	{
		size_t idx = ASContextPool_GetFreeContextIndex(&manager->ctx_pool);
		asIScriptContext* ctx = reinterpret_cast<asIScriptContext*>(ASContextPool_GetContextAt(&manager->ctx_pool, idx));
		ctx->Prepare(reinterpret_cast<asIScriptFunction*>(manager->on_player_connect_func));
		asIScriptObject* playerobj = CreatePlayerProxy(manager, pClient);

		pClient->player_obj = playerobj;

		ctx->SetArgObject(0, playerobj);
		ctx->Execute();

		ASContextPool_ReturnContextByIndex(&manager->ctx_pool, idx);
	}

	void AngelScriptManager_RunWorldTick(AngelScriptManager* manager)
	{
		asIScriptContext* ctx = reinterpret_cast<asIScriptContext*>(manager->world_tick_scriptcontext);
		ctx->Prepare(reinterpret_cast<asIScriptFunction*>(manager->world_tick_func));
		ctx->Execute();
	}

	void AngelScriptManager_ReleaseEngine(AngelScriptManager* manager)
	{
		reinterpret_cast<asIScriptContext*>(manager->world_tick_scriptcontext)->Release();
		ASContextPool_Destroy(&manager->ctx_pool);
		reinterpret_cast<asIScriptEngine*>(manager->engine)->Release();
		delete reinterpret_cast<asCJITCompiler*>(manager->jit);

		MemoryPool_Destroy(&manager->mem_pool);
	}

	int ASContextPool_Init(ASContextPool* pPool, void* engine, size_t initial_size)
	{
		asIScriptEngine* pEngine = reinterpret_cast<asIScriptEngine*>(engine);
		pthread_mutex_init(&pPool->mtx, 0);
		pPool->pEngine = pEngine;
		pPool->pContextArray = (void**) talloc(sizeof(asIScriptContext*) * initial_size);
		pPool->pInUseArray = (unsigned char*) talloc(sizeof(unsigned char) * initial_size);

		RETURNFAIL_IF(!pPool->pContextArray || !pPool->pInUseArray);

		memset(pPool->pContextArray, 0, sizeof(asIScriptContext*) * initial_size);
		memset(pPool->pInUseArray, 0, sizeof(unsigned char) * initial_size);


		pPool->ctx_count = initial_size;
		size_t idx = 0;
		for(; idx < initial_size; ++idx)
		{
			pPool->pContextArray[idx] = pEngine->CreateContext();
		}

		return 0;
	}

	void* ASContextPool_GetContextAt(ASContextPool* pPool, size_t idx)
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
			pPool->pContextArray = (void**) trealloc(pPool->pContextArray, newsize * sizeof(asIScriptContext*));
			pPool->pInUseArray = (unsigned char*) trealloc(pPool->pInUseArray, newsize * sizeof(unsigned char));

			memset(&pPool->pInUseArray[oldsize], 0, sizeof(unsigned char) * (newsize - oldsize));
			size_t idx = oldsize;
			for(; idx < newsize; ++idx)
			{
				pPool->pContextArray[idx] = reinterpret_cast<asIScriptEngine*>(pPool->pEngine)->CreateContext();
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
			reinterpret_cast<asIScriptContext*>(pPool->pContextArray[idx])->Release();
		}
		tfree(pPool->pContextArray);
		tfree(pPool->pInUseArray);
		pthread_mutex_destroy(&pPool->mtx);
	}
}
