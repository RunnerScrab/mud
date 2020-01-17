#include "as_manager.h"
#include "./angelscriptsdk/sdk/angelscript/source/scriptstdstring.h"
#include "./angelscriptsdk/sdk/angelscript/source/scriptarray.h"
#include <cstdio>
#include <cstdlib>
#include "server.h"
#include "as_api.h"
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
	size_t idx = 0, len = manager->main_module->GetGlobalVarCount();
	for(; idx < len; ++idx)
	{
		manager->main_module->RemoveGlobalVar(idx);
	}

	manager->engine->Release();
	delete manager->jit;

	MemoryPool_Destroy(&manager->mem_pool);
}
