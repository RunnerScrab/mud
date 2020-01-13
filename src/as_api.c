#include "as_api.h"

void ASAPI_SendToAll(struct Server* server, std::string& message)
{
	Server_SendAllClients(server, message.c_str());
}

void* ASAPI_RunScriptCommand(void* pArgs)
{

	return (void*) 0;
}

void ASAPI_QueueScriptCommand(struct Server* server, asIScriptObject* obj, unsigned int delay)
{
	if(obj)
	{
		struct ThreadTask* pTask = (struct ThreadTask*) MemoryPool_Alloc(&server->mem_pool,
										sizeof(struct ThreadTask));
		pTask->taskfn = ASAPI_RunScriptCommand;
		pTask->pArgs = obj;
		pTask->releasefn = 0;
	}
}
