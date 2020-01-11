#ifndef ANGELSCRIPT_MANAGER_H
#define ANGELSCRIPT_MANAGER_H
#include "./angelscriptsdk/sdk/angelscript/include/angelscript.h"
#include "./angelscriptsdk/sdk/angelscript/source/as_jit.h"

extern "C" typedef struct
{
	asIScriptEngine* engine;
	asCJITCompiler* jit;

	asIScriptModule* main_module;

	asIScriptFunction* world_tick_func;
	asIScriptContext* world_tick_scriptcontext;
}
AngelScriptManager;

extern "C" int AngelScriptManager_InitEngine(AngelScriptManager* manager);
extern "C" int AngelScriptManager_LoadScripts(AngelScriptManager* manager, const char* script_dir);
extern "C" int AngelScriptManager_InitAPI(AngelScriptManager* manager, struct Server* server);
extern "C" void AngelScriptManager_RunWorldTick(AngelScriptManager* manager);
extern "C" void AngelScriptManager_ReleaseEngine(AngelScriptManager* manager);

#endif
