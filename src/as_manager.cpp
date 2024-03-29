#include "as_manager.h"
extern "C"
{
#include "server.h"
#include "utils.h"
#include "serverconfig.h"
#include "talloc.h"
}
#include "as_api.h"
#include "action_scheduler.h"
#include "nativeactor.h"
#include "player.h"
#include "uuid.h"
#include "sqlitetable.h"
#include "mpnumbers.h"
#include "editabletext.h"
#include "leditor.h"
#include "commandlexer.h"

#include "as_addons/scriptdictionary.h"
#include "as_addons/scriptmath.h"
#include "as_addons/scriptstdstring.h"
#include "as_addons/scriptarray.h"
#include "as_addons/scripthelper.h"
#include "as_addons/scripthandle.h"
#include "as_addons/weakref.h"
#include "./angelscriptsdk/sdk/angelscript/include/angelscript.h"
#include "./angelscriptsdk/sdk/angelscript/source/as_jit.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <functional>
#include <pthread.h>
#include <memory>
#include <dirent.h>

extern "C"
{
	void as_MessageCallback(const asSMessageInfo *msg, void *param)
	{
		const char *type = "ERR ";
		if (msg->type == asMSGTYPE_WARNING)
			type = "WARN";
		else if (msg->type == asMSGTYPE_INFORMATION)
			type = "INFO";

		ServerLog(SERVERLOG_ERROR, "Angelscript: %s (%d, %d) : %s : %s\n",
			  msg->section, msg->row, msg->col, type, msg->message);
	}

	int AngelScriptManager_InitEngine(AngelScriptManager *manager,
					  struct Server *server)
	{
		MemoryPool_Init(&manager->mem_pool, 32);
		manager->lexer = new CommandLexer();
		manager->server = server;
		manager->engine = asCreateScriptEngine();
		manager->engine->SetEngineProperty(asEP_BUILD_WITHOUT_LINE_CUES, true);
		manager->engine->SetEngineProperty(asEP_USE_CHARACTER_LITERALS, true);
#ifdef __x86_64__
		ServerLog(SERVERLOG_STATUS, "x86_64 Build. Enabling Angelscript JIT module.");
		manager->jit = new asCJITCompiler(0);
		manager->engine->SetEngineProperty(asEP_INCLUDE_JIT_INSTRUCTIONS, 1);
		manager->engine->SetJITCompiler(manager->jit);
#endif
		RETURNFAIL_IF(!manager->engine);

		manager->engine->SetEngineProperty(asEP_ALLOW_MULTILINE_STRINGS, true);

		RegisterScriptMath(manager->engine);
		RegisterStdString(manager->engine);
		RegisterScriptArray(manager->engine, true);
		RegisterScriptDictionary(manager->engine);
		RegisterScriptHandle(manager->engine);
		RegisterScriptWeakRef(manager->engine);
		RegisterExceptionRoutines(manager->engine);

		manager->engine->SetMessageCallback(asFUNCTION(as_MessageCallback), 0,
						    asCALL_CDECL);

		ASContextPool_Init(&manager->ctx_pool, manager->engine, manager, 32);

		manager->action_scheduler = new ActionScheduler(server);
		manager->action_scheduler->StartThread();
		return 0;
	}

	void AngelScriptManager_ReleaseEngine(AngelScriptManager *manager)
	{
		manager->action_scheduler->StopThread();
		delete manager->action_scheduler;
		manager->action_scheduler = 0;

		if (manager->world_tick_scriptcontext)
		{
			manager->world_tick_scriptcontext->Release();
		}
		ASContextPool_Destroy(&manager->ctx_pool);
		manager->engine->Release();

#if __x86_64__
		delete manager->jit;
		manager->jit = 0;
#endif
		delete manager->lexer;
		manager->lexer = 0;

		MemoryPool_Destroy(&manager->mem_pool);
		memset(manager, 0, sizeof(AngelScriptManager));
	}

	static size_t GetFileLength(FILE *fp)
	{
		fseek(fp, 0, SEEK_END);
		size_t len = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		return len;
	}

	static int CheckExtension(const char* str, size_t len, const char* ext, size_t ext_len)
	{
		size_t minlen = len < ext_len ? len : ext_len;
		return 0 == strncmp(ext, &str[len - ext_len], minlen) ? 1 : 0;
	}

	asIScriptModule* AngelScriptManager_GetMainModule(AngelScriptManager* manager)
	{
		return manager->main_module;
	}

	int AngelScriptManager_InitAPI(AngelScriptManager *manager)
	{
		int result = 0;
		struct Server *server = manager->server;
		asIScriptEngine *pEngine = manager->engine;

		result = RegisterLexerClasses(manager->engine);
		RETURNFAIL_IF(result < 0);

		result = RegisterEditableTextClass(manager->engine);
		RETURNFAIL_IF(result < 0);

		result = RegisterMPNumberClasses(manager->engine);
		RETURNFAIL_IF(result < 0);

		result = RegisterUUIDClass(manager);
		RETURNFAIL_IF(result < 0);

		result = pEngine->RegisterInterface("IAction");
		RETURNFAIL_IF(result < 0);

		result = pEngine->RegisterInterfaceMethod("IAction", "int opCall()");
		RETURNFAIL_IF(result < 0);

		result = RegisterNativeActorClass(pEngine, manager);
		RETURNFAIL_IF(result < 0);

		ServerLog(SERVERLOG_STATUS, "Registered actor class.");

		result = RegisterPlayerConnectionClass(pEngine);
		RETURNFAIL_IF(result < 0);

		result = RegisterLineEditorClass(pEngine);
		RETURNFAIL_IF(result < 0);

		result = pEngine->RegisterObjectType("Server", 0,
						     asOBJ_REF | asOBJ_NOCOUNT);
		RETURNFAIL_IF(result < 0);

		result = pEngine->RegisterObjectMethod("Server",
						       "void SendToAll(string& in)", asFUNCTION(ASAPI_SendToAll),
						       asCALL_CDECL_OBJFIRST);
		RETURNFAIL_IF(result < 0);

		result = pEngine->RegisterObjectMethod("Server",
						       "void QueueGlobalAction(IAction@+ cmd, uint32 delay)",
						       asFUNCTION(ASAPI_QueueScriptAction), asCALL_CDECL_OBJFIRST);
		RETURNFAIL_IF(result < 0);

		result = pEngine->RegisterObjectMethod("Server",
						       "void DebugVariables(PlayerConnection@ player)",
						       asFUNCTION(ASAPI_DebugVariables), asCALL_CDECL_OBJFIRST);
		RETURNFAIL_IF(result < 0);

		result = pEngine->RegisterObjectMethod("Server",
						       "void Kill()",
						       asFUNCTION(ASAPI_KillServer), asCALL_CDECL_OBJFIRST);
		RETURNFAIL_IF(result < 0);

		result = pEngine->RegisterObjectMethod("Server",
						       "void Reload()",
						       asFUNCTION(ASAPI_ReloadServer), asCALL_CDECL_OBJFIRST);
		RETURNFAIL_IF(result < 0);

		result = pEngine->RegisterGlobalFunction("void DebugObject(ref@ obj)",
							 asFUNCTION(ASAPI_DebugObject), asCALL_CDECL);
		RETURNFAIL_IF(result < 0);

		result = pEngine->SetDefaultNamespace("Global");
		RETURNFAIL_IF(result < 0);

		result = pEngine->RegisterGlobalProperty("Server game_server", server);
		RETURNFAIL_IF(result < 0);

		result = pEngine->RegisterGlobalProperty("CommandLexer lexer", manager->lexer);
		RETURNFAIL_IF(result < 0);

		result = pEngine->SetDefaultNamespace("");
		RETURNFAIL_IF(result < 0);

		result = pEngine->RegisterGlobalFunction("void Log(string& in)",
							 asFUNCTION(ASAPI_Log), asCALL_CDECL);
		RETURNFAIL_IF(result < 0);

		result = pEngine->RegisterGlobalFunction(
			"string TrimString(const string& in)", asFUNCTION(ASAPI_TrimString),
			asCALL_CDECL);

		RETURNFAIL_IF(result < 0);

		result = pEngine->RegisterGlobalFunction(
			"string HashPassword(const string& in)",
			asFUNCTION(ASAPI_HashPassword), asCALL_CDECL);

		RETURNFAIL_IF(result < 0);

		return 0;
	}

	int AngelScriptManager_LoadServerConfig(AngelScriptManager *manager,
						struct ServerConfig *serverconfig)
	{
		//Rather than create an ad-hoc domain specific configuration language for the server,
		//let's just use AngelScript since we've already got it.
		//This needs to be called before AngelScriptManager_LoadScripts()
		int result = 0;
		std::string script;
		std::string scriptpath = "./server.cfg";
		FILE *fp = fopen(scriptpath.c_str(), "rb");
		RETURNFAIL_IF(!fp);
		dbgprintf("Opened server.cfg\n");

		size_t len = GetFileLength(fp);

		script.resize(len);
		RETURNFAIL_IF(fread(&script[0], len, 1, fp) <= 0);
		fclose(fp);

		manager->config_module = manager->engine->GetModule("config_module",
								    asGM_ALWAYS_CREATE);
		RETURNFAIL_IF(
			manager->config_module->AddScriptSection("server_configuration",
								 &script[0], len) < 0);

		result = manager->engine->RegisterObjectType("ServerConfig", 0,
							     asOBJ_REF | asOBJ_NOCOUNT);
		RETURNFAIL_IF(result < 0);

		result = manager->engine->RegisterObjectMethod("ServerConfig",
							       "void SetDatabasePathAndFile(string& in, string& in)",
							       asFUNCTION(ASAPI_SetDatabasePathAndFile), asCALL_CDECL_OBJFIRST);
		RETURNFAIL_IF(result < 0);
		result = manager->engine->RegisterObjectMethod("ServerConfig",
							       "void SetGameScriptPath(string& in)",
							       asFUNCTION(ASAPI_SetGameScriptPath), asCALL_CDECL_OBJFIRST);
		RETURNFAIL_IF(result < 0);
		result = manager->engine->RegisterObjectMethod("ServerConfig",
							       "void SetGameBindAddress(string& in, uint16 port)",
							       asFUNCTION(ASAPI_SetGameBindAddress), asCALL_CDECL_OBJFIRST);
		RETURNFAIL_IF(result < 0);

		RETURNFAIL_IF(manager->config_module->Build() < 0);

		manager->server_setup_func = manager->config_module->GetFunctionByDecl(
			"void SetupServer(ServerConfig@ config)");

		RETURNFAIL_IF(0 == manager->server_setup_func);

		size_t ctxidx = ASContextPool_GetFreeContextIndex(&manager->ctx_pool);
		asIScriptContext *ctx = ASContextPool_GetContextAt(&manager->ctx_pool,
								   ctxidx);
		ctx->Prepare(manager->server_setup_func);
		ctx->SetArgObject(0, serverconfig);
		ctx->Execute();
		ASContextPool_ReturnContextByIndex(&manager->ctx_pool, ctxidx);

		return 0;
	}

	void AngelScriptManager_CleanTypeSchemaUserData(asITypeInfo *pType)
	{
		dbgprintf("In cleanup function for type %s\n", pType->GetName());
		SQLiteTable *pTable = reinterpret_cast<SQLiteTable*>(pType->GetUserData(
									     AS_USERDATA_TYPESCHEMA));
		if (pTable)
		{
			dbgprintf("Freeing %s's userdata.\n", pType->GetName());
			delete pTable;
		}
	}

	int AngelScriptManager_PrepareScriptPersistenceLayer(
		AngelScriptManager *manager)
	{
		//This isn't done at the same time as the Database API initialization
		//because this requires the scripts to be loaded, and the database API needs to be initialized
		//before scripts are loaded

		//TODO: Remove this - debug script class information
		dbgprintf("-Class information-\n");
		size_t idx = 0, object_type_count =
			manager->main_module->GetObjectTypeCount();
		asITypeInfo *pPersistentType = manager->engine->GetTypeInfoByName(
			"IPersistent");

		if (!pPersistentType)
		{
			ServerLog(SERVERLOG_ERROR,
				  "Could not find IPersistent interface type!");
			return -1;
		}
		else
		{
			dbgprintf("Found IPersistent interface type.");
		}

		//Set the cleanup callback function for all the tables we'll
		//store with their types

		size_t ctxidx = ASContextPool_GetFreeContextIndex(&manager->ctx_pool);
		asIScriptContext *ctx = ASContextPool_GetContextAt(&manager->ctx_pool,
								   ctxidx);
		sqlite3 *pSQLiteDB = SQLiteTable::GetDBConnection();

		manager->engine->SetTypeInfoUserDataCleanupCallback(
			AngelScriptManager_CleanTypeSchemaUserData,
			AS_USERDATA_TYPESCHEMA);

		std::vector<asITypeInfo*> pPersistentTypes;
		pPersistentTypes.reserve(object_type_count);
		for (idx = 0; idx < object_type_count; ++idx)
		{
			//Precreate table objects so that the table structure definition
			//functions called below will have access to all their names
			asITypeInfo *pInfo = manager->main_module->GetObjectTypeByIndex(idx);
			if (pInfo->Implements(pPersistentType))
			{
				dbgprintf("CREATING a new table for class %s.\n", pInfo->GetName());
				SQLiteTable *pTable = new SQLiteTable(pSQLiteDB, pInfo->GetName());
				pTable->AddRef();
				pInfo->SetUserData((void*) pTable, AS_USERDATA_TYPESCHEMA);
				pPersistentTypes.push_back(pInfo);
			}
		}

		for (asITypeInfo *pInfo : pPersistentTypes)
		{
			SQLiteTable *pTable = (SQLiteTable*) pInfo->GetUserData(AS_USERDATA_TYPESCHEMA);
			//We want to reuse this table for every parent class of the type in pInfo.
			///Every class and subclass will have its own table which includes all the
			//columns of its parent classes.
			while (pInfo)
			{
				//Do not get the virtual implementation
				asIScriptFunction *pDSfun = pInfo->GetMethodByName("OnDefineSchema",
										   false);
				if (pDSfun)
				{
					dbgprintf("Found %s's OnDefineSchema()", pInfo->GetName());
					ctx->Prepare(pDSfun);
					ctx->SetObject(0);
					ctx->SetArgObject(0, pTable);
					ctx->Execute();

					//Breaking here stops automatic calling of superclass IPersistent
					//OnDefineSchema, which is against convention and confusing to
					//people experienced with OOP
					break;
				}
				pInfo = pInfo->GetBaseType();
				if (pInfo)
				{
					dbgprintf("Found base type %s", pInfo->GetName());
				}
				else
				{
					dbgprintf("Did not find base type.");
				}
			}
		}

		ASContextPool_ReturnContextByIndex(&manager->ctx_pool, ctxidx);
		return 0;
	}

	int AngelScriptManager_LoadScripts(AngelScriptManager *manager,
					   const char *script_dir)
	{

		DIR* pDir = 0;
		struct dirent* pDirEntry = 0;
		pDir = opendir(script_dir);
		RETURNFAIL_IF(pDir == 0);

		std::vector<std::string> asfiles;
		for(;;)
		{
			pDirEntry = readdir(pDir);
			if(!pDirEntry)
			{
				break;
			}
			//d_name is a maximum of 256 bytes
			if(CheckExtension(pDirEntry->d_name, strnlen(pDirEntry->d_name, 256),
					  ".as", 3))
			{
				asfiles.push_back(std::string(pDirEntry->d_name));
			}
		}
		closedir(pDir);

		//TODO: May want to impose some kind of directory structure on scripts

		if(manager && manager->engine)
		{
		manager->main_module = manager->engine->GetModule("game_module",
								  asGM_ALWAYS_CREATE);
		}
		else
		{
			ServerLog(SERVERLOG_ERROR, "Trying to dereference a null engine pointer!");
			return -1;
		}

		RETURNFAIL_IF(LoadActorProxyScript(manager->main_module));

		std::string script;
		for(std::string& name : asfiles)
		{
			script.clear();
			std::string scriptpath = std::string(script_dir) + name;
			FILE *fp = fopen(scriptpath.c_str(), "rb");
			RETURNFAIL_IF(!fp);

			size_t len = GetFileLength(fp);

			script.resize(len);

			RETURNFAIL_IF(fread(&script[0], len, 1, fp) <= 0);

			fclose(fp);

			printf("LOADING SCRIPT FILE: %s\n", name.c_str());
			RETURNFAIL_IF(
				manager->main_module->AddScriptSection(name.c_str(), &script[0], len) < 0);

		}


		RETURNFAIL_IF(manager->main_module->Build() < 0);

#ifdef DEBUG
		size_t global_properties = manager->engine->GetGlobalPropertyCount();
		dbgprintf("There are %lu global properties.\n", global_properties);
#endif

#ifdef __x86_64__
		manager->jit->finalizePages();
#endif
		manager->server_init_func = manager->main_module->GetFunctionByDecl(
			"int OnServerStart()");
		RETURNFAIL_IF(0 == manager->server_init_func);

		manager->world_tick_func = manager->main_module->GetFunctionByDecl(
			"void GameTick()");
		RETURNFAIL_IF(0 == manager->world_tick_func);

		manager->on_player_connect_func = manager->main_module->GetFunctionByDecl(
			"void OnPlayerConnect(PlayerConnection@ player)");
		RETURNFAIL_IF(0 == manager->on_player_connect_func);

		manager->world_tick_scriptcontext = manager->engine->CreateContext();

		return 0;
	}

	void AngelScriptManager_CallOnPlayerDisconnect(AngelScriptManager *manager,
						       struct Client *pClient)
	{
		dbgprintf("Calling on player disconnect.\n");
		PlayerConnection *pPlayer =
			reinterpret_cast<PlayerConnection*>(pClient->player_obj);
		asIScriptFunction *fun = pPlayer->GetDisconnectCallback();

		if (fun)
		{
			//Call player disconnect callback
			size_t idx = ASContextPool_GetFreeContextIndex(&manager->ctx_pool);
			asIScriptContext *ctx = ASContextPool_GetContextAt(&manager->ctx_pool,
									   idx);

			ctx->Prepare(fun);
			ctx->Execute();

			ASContextPool_ReturnContextByIndex(&manager->ctx_pool, idx);
		}

	}

	int AngelScriptManager_CallOnServerStartFunc(AngelScriptManager* manager)
	{
		//The init function of the scripts
		size_t ctxidx = ASContextPool_GetFreeContextIndex(&manager->ctx_pool);
		asIScriptContext *ctx = ASContextPool_GetContextAt(&manager->ctx_pool,
								   ctxidx);
		ctx->Prepare(manager->server_init_func);
		ctx->Execute();
		int retval = ctx->GetReturnDWord();
		ASContextPool_ReturnContextByIndex(&manager->ctx_pool, ctxidx);
		return retval;
	}

	void AngelScriptManager_CallOnPlayerInput(AngelScriptManager *manager,
						  struct Client *pClient, const char *input)
	{
		PlayerConnection *pPlayer =
			reinterpret_cast<PlayerConnection*>(pClient->player_obj);
		asIScriptFunction *fun = pPlayer->GetInputCallback();

		if (fun)
		{
			size_t idx = ASContextPool_GetFreeContextIndex(&manager->ctx_pool);
			asIScriptContext *ctx = ASContextPool_GetContextAt(&manager->ctx_pool,
									   idx);
			std::string temp(input);
			ctx->Prepare(fun);
			ctx->SetArgObject(0, &temp);
			ctx->Execute();
			ASContextPool_ReturnContextByIndex(&manager->ctx_pool, idx);
		}

	}

	void AngelScriptManager_CallOnPlayerConnect(AngelScriptManager *manager,
						    struct Client *pClient)
	{
		size_t idx = ASContextPool_GetFreeContextIndex(&manager->ctx_pool);
		asIScriptContext *ctx = ASContextPool_GetContextAt(&manager->ctx_pool, idx);
		ctx->Prepare(manager->on_player_connect_func);
		PlayerConnection *playerobj = PlayerConnection::Factory(pClient);
		if (!playerobj)
		{
			ServerLog(SERVERLOG_ERROR, "Failed to create playerconnection object!");
		}
		else
		{
			pClient->player_obj = playerobj;
			ctx->SetArgObject(0, playerobj);
			ctx->Execute();
		}
		ASContextPool_ReturnContextByIndex(&manager->ctx_pool, idx);

	}

	void AngelScriptManager_RunWorldTick(AngelScriptManager *manager)
	{
		manager->world_tick_scriptcontext->Prepare(manager->world_tick_func);
		manager->world_tick_scriptcontext->Execute();
	}

	void AngelScriptManager_OnScriptException(AngelScriptManager *manager,
						  asIScriptContext *ctx)
	{
		const char *section_name = 0;
		int col_number = 0;
		int line_number = ctx->GetExceptionLineNumber(&col_number, &section_name);
		asIScriptFunction *func = ctx->GetExceptionFunction();
		ServerLog(SERVERLOG_ERROR,
			  "SCRIPT EXCEPTION '%s' thrown from %s:%s:(%d, %d)!",
			  ctx->GetExceptionString(), section_name, func->GetDeclaration(),
			  line_number, col_number);
	}

	int ASContextPool_Init(ASContextPool *pPool, asIScriptEngine *pEngine,
			       AngelScriptManager *pManager, size_t initial_size)
	{
		pthread_mutex_init(&pPool->mtx, 0);
		pPool->pEngine = pEngine;
		pPool->pContextArray = (asIScriptContext**) talloc(
			sizeof(asIScriptContext*) * initial_size);
		pPool->pInUseArray = (unsigned char*) talloc(
			sizeof(unsigned char) * initial_size);

		RETURNFAIL_IF(!pPool->pContextArray || !pPool->pInUseArray);

		memset(pPool->pContextArray, 0, sizeof(asIScriptContext*) * initial_size);
		memset(pPool->pInUseArray, 0, sizeof(unsigned char) * initial_size);

		pPool->ctx_count = initial_size;
		size_t idx = 0;
		for (; idx < initial_size; ++idx)
		{
			asIScriptContext *ctx = pEngine->CreateContext();
			ctx->SetExceptionCallback(
				asFUNCTION(AngelScriptManager_OnScriptException), pManager,
				asCALL_CDECL_OBJFIRST);

			pPool->pContextArray[idx] = ctx;
		}

		return 0;
	}

	asIScriptContext* ASContextPool_GetContextAt(ASContextPool *pPool, size_t idx)
	{
		if (idx > pPool->ctx_count)
			return 0;

		return pPool->pContextArray[idx];
	}

	void ASContextPool_ReturnContextByIndex(ASContextPool *pPool, size_t idx)
	{
		//Mark context as usable again.
		pPool->pInUseArray[idx] = (unsigned char) 0;
	}

	size_t ASContextPool_GetFreeContextIndex(ASContextPool *pPool)
	{
		pthread_mutex_lock(&pPool->mtx);
		unsigned char *pLoc = (unsigned char*) memchr(pPool->pInUseArray, 0,
							      pPool->ctx_count);
		if (!pLoc)
		{
			size_t oldsize = pPool->ctx_count;
			size_t newsize = oldsize << 1;
			pPool->pContextArray = (asIScriptContext**) trealloc(
				pPool->pContextArray, newsize * sizeof(asIScriptContext*));
			pPool->pInUseArray = (unsigned char*) trealloc(pPool->pInUseArray,
								       newsize * sizeof(unsigned char));

			memset(&pPool->pInUseArray[oldsize], 0,
			       sizeof(unsigned char) * (newsize - oldsize));
			size_t idx = oldsize;
			for (; idx < newsize; ++idx)
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

	void ASContextPool_Destroy(ASContextPool *pPool)
	{
		pthread_mutex_lock(&pPool->mtx);
		size_t idx = 0, len = pPool->ctx_count;
		for (; idx < len; ++idx)
		{
			pPool->pContextArray[idx]->Release();
		}
		tfree(pPool->pContextArray);
		tfree(pPool->pInUseArray);
		pthread_mutex_destroy(&pPool->mtx);
	}
}
