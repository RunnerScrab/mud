#include "as_manager.h"
#include "server.h"
#include "as_api.h"
#include "talloc.h"
#include "./angelscriptsdk/sdk/angelscript/source/scriptstdstring.h"
#include "./angelscriptsdk/sdk/angelscript/source/scriptarray.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <functional>
#include <pthread.h>
#include <memory>

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
	manager->jit = new asCJITCompiler(0);

	manager->engine->SetEngineProperty(asEP_INCLUDE_JIT_INSTRUCTIONS, 1);
	manager->engine->SetJITCompiler(manager->jit);
	if(!manager->engine)
	{
		return -1;
	}

	RegisterStdString(manager->engine);
	RegisterScriptArray(manager->engine, true);
	manager->engine->SetMessageCallback(asFUNCTION(as_MessageCallback), 0, asCALL_CDECL);
	ASContextPool_Init(&manager->ctx_pool, manager->engine, 1);
	return 0;
}

int AngelScriptManager_InitAPI(AngelScriptManager* manager, struct Server* server)
{
	int result = 0;
	asIScriptEngine* pEngine = manager->engine;

	result = pEngine->RegisterObjectType("Server", 0, asOBJ_REF | asOBJ_NOCOUNT);
	if(result < 0)
	{
		return -1;
	}

	result = pEngine->RegisterObjectMethod("Server", "void SendToAll(string& in)",
						asFUNCTION(ASAPI_SendToAll), asCALL_CDECL_OBJFIRST);
	if(result < 0)
	{
		return -1;
	}

	result = pEngine->RegisterInterface("ICommand");

	if(result < 0)
	{
		return -1;
	}

	pEngine->RegisterInterfaceMethod("ICommand", "int opCall()");

	if(result < 0)
	{
		return -1;
	}

	result = pEngine->RegisterObjectMethod("Server", "void QueueScriptCommand(ICommand@+ cmd, uint32 delay)",
					asFUNCTION(ASAPI_QueueScriptCommand), asCALL_CDECL_OBJFIRST);

	if(result < 0)
	{
		return -1;
	}

	result = pEngine->RegisterGlobalFunction("void Log(string& in)", asFUNCTION(ASAPI_Log), asCALL_CDECL);

	if(result < 0)
	{
		return -1;
	}

	result = pEngine->RegisterGlobalProperty("Server game_server", server);
	if(result < 0)
	{
		return -1;
	}


	return 0;
}

int AngelScriptManager_LoadScripts(AngelScriptManager* manager, const char* script_dir)
{
	//TODO: May want to impose some kind of directory structure on scripts
	std::string script;
	std::string scriptpath = std::string(script_dir) + "/test.as";
	FILE* fp = fopen(scriptpath.c_str(), "rb");
	if(!fp)
	{
		return -1;
	}
	fseek(fp, 0, SEEK_END);
	size_t len = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	script.resize(len);

	if(fread(&script[0], len, 1, fp) <= 0)
	{
		return -1;
	}
	fclose(fp);

	manager->main_module = manager->engine->GetModule(0, asGM_ALWAYS_CREATE);
	if(manager->main_module->AddScriptSection("game", &script[0], len) < 0)
	{
		return -1;
	}

	if(manager->main_module->Build() < 0)
	{
		return -1;
	}

	manager->jit->finalizePages();

	manager->world_tick_func = manager->engine->GetModule(0)->GetFunctionByDecl("void GameTick()");
	if(0 == manager->world_tick_func)
	{
		return -1;
	}

	manager->world_tick_scriptcontext = manager->engine->CreateContext();

	return 0;
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
	delete manager->jit;

	MemoryPool_Destroy(&manager->mem_pool);
}

int ASContextPool_Init(ASContextPool* pPool, asIScriptEngine* pEngine, size_t initial_size)
{
	pthread_mutex_init(&pPool->mtx, 0);
	pPool->pEngine = pEngine;
	pPool->pContextArray = (asIScriptContext**) talloc(sizeof(asIScriptContext*) * initial_size);
	pPool->pInUseArray = (unsigned char*) talloc(sizeof(unsigned char) * initial_size);

	if(!pPool->pContextArray || !pPool->pInUseArray)
	{
		return -1;
	}
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

asIScriptContext* ASContextPool_GetContextAt(ASContextPool* pPool, size_t idx)
{
	if(idx > pPool->ctx_count)
		return 0;

	return pPool->pContextArray[idx];
}

void ASContextPool_ReturnContextByIndex(ASContextPool* pPool, size_t idx)
{
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
