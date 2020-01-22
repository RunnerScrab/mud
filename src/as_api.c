#include "as_api.h"
#include "poolalloc.h"
#include <ctype.h>

void ASAPI_SendToAll(struct Server* server, std::string& message)
{
	Server_SendAllClients(server, message.c_str());
}


struct RunScriptCmdPkg
{
	asITypeInfo* cmdtype;
	asIScriptObject* cmd;
	size_t context_handle;
	asIScriptEngine* engine;
	ASContextPool* context_pool;
	MemoryPool* pMemPool; //Memory pool space for this struct was allocated on
};

void* ASAPI_RunScriptCommand(void* pArgs)
{
	struct RunScriptCmdPkg* pPkg = (struct RunScriptCmdPkg*) pArgs;
	asIScriptFunction* func = pPkg->cmdtype->GetMethodByDecl("int opCall()");
	ASContextPool* pctx_pool = pPkg->context_pool;

	//The faster alternative to a context pool would be giving the
	//threadpool workers each their own context they would reuse
	//for every task they ran which required it. This would
	//require differentiating between a script task and an
	//internally generated task everywhere tasks are handled,
	//however, resulting in a slightly less flexible design.

	asIScriptContext* context = ASContextPool_GetContextAt(pctx_pool, pPkg->context_handle);
	context->Prepare(func);
	context->SetObject(pPkg->cmd);
	context->Execute();
	ASContextPool_ReturnContextByIndex(pctx_pool, pPkg->context_handle);
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
		pkg->engine = server->as_manager.engine;
		size_t hfreectx = ASContextPool_GetFreeContextIndex(&server->as_manager.ctx_pool);
		pkg->context_pool = &server->as_manager.ctx_pool;
		pkg->context_handle = hfreectx;
		Server_AddTimedTask(server, ASAPI_RunScriptCommand,
				time(0) + delay, pkg, 0);
	}
}

void ASAPI_Log(std::string& message)
{
	ServerLog(SERVERLOG_STATUS, message.c_str());
}

std::string ASAPI_TrimString(const std::string& buf)
{
	size_t idx = buf.length() - 1;
	for(; idx > 0 && isspace(buf[idx]); --idx);
	return buf.substr(0, idx + 1);
}
