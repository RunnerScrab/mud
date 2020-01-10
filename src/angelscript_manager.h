#ifndef ANGELSCRIPT_MANAGER_H
#define ANGELSCRIPT_MANAGER_H
#include "./angelscriptsdk/angelscript/include/angelscript.h"
#include "./angelscriptsdk/angelscript/include/as_jit.h"

extern "C" typedef struct
{
	asIScriptEngine* engine;
	asCJITCompiler* jit;

	asIScriptModule* main_module;

	asIScriptContext* world_tick_scriptcontext;
}
AngelScriptManager;

extern "C" int AngelScriptManager_InitEngine(AngelScriptManager* manager);
extern "C" int AngelScriptManager_LoadScripts(AngelScriptManager* manager, const char* script_dir);
extern "C" void AngelScriptManager_RunWorldTick(AngelScriptManager* manager);
extern "C" void AngelScriptManager_ReleaseEngine(AngelScriptManager* manager);


#endif
