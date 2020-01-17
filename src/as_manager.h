#ifndef ANGELSCRIPT_MANAGER_H
#define ANGELSCRIPT_MANAGER_H
#include "./angelscriptsdk/sdk/angelscript/include/angelscript.h"
#include "./angelscriptsdk/sdk/angelscript/source/as_jit.h"
#include "poolalloc.h"
#include <vector>
#include <pthread.h>

extern "C" typedef struct
{
	asIScriptEngine* engine;
	asCJITCompiler* jit;

	asIScriptModule* main_module;

	asIScriptFunction* world_tick_func;
	asIScriptContext* world_tick_scriptcontext;

	MemoryPool mem_pool;
}
AngelScriptManager;

extern "C" int AngelScriptManager_InitEngine(AngelScriptManager* manager);
extern "C" int AngelScriptManager_LoadScripts(AngelScriptManager* manager, const char* script_dir);
extern "C" int AngelScriptManager_InitAPI(AngelScriptManager* manager, struct Server* server);
extern "C" void AngelScriptManager_RunWorldTick(AngelScriptManager* manager);
extern "C" void AngelScriptManager_ReleaseEngine(AngelScriptManager* manager);

asIScriptContext *RequestContextCallback(asIScriptEngine *engine, void *param);
void ReturnContextToPool(asIScriptEngine *engine, asIScriptContext *ctx, void *param);
#endif
