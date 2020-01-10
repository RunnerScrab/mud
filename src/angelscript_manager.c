#include "angelscript_manager.h"
#include "./angelscriptsdk/add_on/scriptstdstring/scriptstdstring.h"
#include "./angelscriptsdk/add_on/scriptarray/scriptarray.h"
#include <cstdio>
#include <cstdlib>
#include "server.h"
#include <string>

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

	asIScriptFunction* worldtickfunc = manager->engine->GetModule(0)->GetFunctionByDecl("void GameTick()");
	if(0 == worldtickfunc)
	{
		return -1;
	}
	manager->world_tick_scriptcontext = manager->engine->CreateContext();
	manager->world_tick_scriptcontext->Prepare(worldtickfunc);
	return 0;
}

void AngelScriptManager_RunWorldTick(AngelScriptManager* manager)
{
	manager->world_tick_scriptcontext->Execute();
}

void AngelScriptManager_ReleaseEngine(AngelScriptManager* manager)
{
	manager->world_tick_scriptcontext->Release();

	manager->engine->Release();
	delete manager->jit;
}
