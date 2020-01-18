#ifndef ANGELSCRIPT_MANAGER_H
#define ANGELSCRIPT_MANAGER_H
#include "./angelscriptsdk/sdk/angelscript/include/angelscript.h"
#include "./angelscriptsdk/sdk/angelscript/source/as_jit.h"
#include "poolalloc.h"
#include <vector>
#include <pthread.h>

extern "C" typedef struct
{
	asIScriptContext** pContextArray;
	unsigned char* pInUseArray;

	size_t ctx_count;

	asIScriptEngine* pEngine;

	pthread_mutex_t mtx;
} ASContextPool;

extern "C" typedef struct
{
	asIScriptEngine* engine;
	asCJITCompiler* jit;

	asIScriptModule* main_module;

	asIScriptFunction* world_tick_func;
	asIScriptContext* world_tick_scriptcontext;

	MemoryPool mem_pool;
	ASContextPool ctx_pool;
}
AngelScriptManager;

extern "C" int AngelScriptManager_InitEngine(AngelScriptManager* manager);
extern "C" int AngelScriptManager_LoadScripts(AngelScriptManager* manager, const char* script_dir);
extern "C" int AngelScriptManager_InitAPI(AngelScriptManager* manager, struct Server* server);
extern "C" void AngelScriptManager_RunWorldTick(AngelScriptManager* manager);
extern "C" void AngelScriptManager_ReleaseEngine(AngelScriptManager* manager);

int ASContextPool_Init(ASContextPool* pPool, asIScriptEngine* pEngine, size_t initial_size);
asIScriptContext* ASContextPool_GetContextAt(ASContextPool* pPool, size_t idx);
void ASContextPool_ReturnContextByIndex(ASContextPool* pPool, size_t idx);
size_t ASContextPool_GetFreeContextIndex(ASContextPool* pPool);
void ASContextPool_Destroy(ASContextPool* pPool);

#endif
