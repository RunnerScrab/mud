#include "as_api.h"
#include "poolalloc.h"

void ASAPI_SendToAll(struct Server* server, std::string& message)
{
	Server_SendAllClients(server, message.c_str());
}


struct RunScriptCmdPkg
{
	asITypeInfo* cmdtype;
	asIScriptObject* cmd;
	asIScriptContext* context;
	asIScriptEngine* engine;
	MemoryPool* pMemPool; //Memory pool space for this struct was allocated on
};

void* ASAPI_RunScriptCommand(void* pArgs)
{
	struct RunScriptCmdPkg* pPkg = (struct RunScriptCmdPkg*) pArgs;
	asIScriptFunction* func = pPkg->cmdtype->GetMethodByDecl("int opCall()");
	pPkg->context = pPkg->engine->CreateContext();
	pPkg->context->Prepare(func);
	pPkg->context->SetObject(pPkg->cmd);
	pPkg->context->Execute();
	pPkg->context->Release();
	printf("Running script function\n");
	MemoryPool_Free(pPkg->pMemPool, sizeof(struct RunScriptCmdPkg), pArgs);

	return (void*) 0;
}

void ASAPI_QueueScriptCommand(struct Server* server, asIScriptObject* obj, unsigned int delay)
{
	printf("Attempting to queue script command.\n");
	if(obj)
	{
		printf("Queueing script command.\n");
		struct RunScriptCmdPkg* pkg = (struct RunScriptCmdPkg*) MemoryPool_Alloc(
			&server->as_manager.mem_pool, sizeof(struct RunScriptCmdPkg));
		pkg->cmd = obj;
		pkg->cmdtype = server->as_manager.main_module->GetTypeInfoByDecl("ICommand");
		pkg->pMemPool = &server->as_manager.mem_pool;
		//	pkg->context = server->as_manager.engine->CreateContext();//Need to find a way to manage contexts, in a pool
		pkg->engine = server->as_manager.engine;
		Server_AddTimedTask(server, ASAPI_RunScriptCommand,
				time(0) + delay, pkg, 0);
	}
}

void ASAPI_Log(std::string& message)
{
	ServerLog(SERVERLOG_STATUS, message.c_str());
}
