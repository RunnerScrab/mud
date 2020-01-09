#ifndef ANGELSCRIPT_MANAGER_H
#define ANGELSCRIPT_MANAGER_H
#include "./angelscriptsdk/angelscript/include/angelscript.h"
#include "./angelscriptsdk/angelscript/include/as_jit.h"

typedef struct
{
	asIScriptEngine* engine;
	asCJITCompiler* jit;

	asIScriptModule* main_module;

	asIScriptContext* world_tick_scriptcontext;
}
AngelScriptManager;

int AngelScriptManager_InitEngine(AngelScriptManager* manager);
int AngelScriptManager_LoadScripts(AngelScriptManager* manager, const char* script_dir);
void AngelScriptManager_RunWorldTick(AngelScriptManager* manager);
void AngelScriptManager_ReleaseEngine(AngelScriptManager* manager);


#endif
