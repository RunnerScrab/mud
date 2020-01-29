#ifndef ANGELSCRIPT_MANAGER_H
#define ANGELSCRIPT_MANAGER_H
#include "poolalloc.h"
#include <pthread.h>

struct Server;
struct ServerConfig;
struct Client;

typedef struct
{
	void** pContextArray;
	unsigned char* pInUseArray;

	size_t ctx_count;

	void* pEngine;

	pthread_mutex_t mtx;
} ASContextPool;

typedef struct
{
	/*
	  asIScriptEngine* engine;
	  asCJITCompiler* jit;

	  asIScriptModule* main_module, *config_module;

	  asIScriptFunction* world_tick_func, *on_player_connect_func;
	  asIScriptFunction* on_player_disconnect_func, *on_player_input_func;
	  asIScriptFunction* server_setup_func;
	  asIScriptContext* world_tick_scriptcontext;
	*/

	void* engine;
	void* jit;
	void* main_module, *config_module;
	void* world_tick_func, *on_player_connect_func;
	void* on_player_disconnect_func, *on_player_input_func;
	void* server_setup_func;
	void* world_tick_scriptcontext;

	struct MemoryPool mem_pool;
	ASContextPool ctx_pool;

	size_t next_free_context_idx;
}
	AngelScriptManager;


int AngelScriptManager_InitEngine(AngelScriptManager* manager);
int AngelScriptManager_LoadServerConfig(AngelScriptManager* manager, struct ServerConfig* server);
int AngelScriptManager_LoadScripts(AngelScriptManager* manager, const char* script_dir);
int AngelScriptManager_InitAPI(AngelScriptManager* manager, struct Server* server);
void AngelScriptManager_RunWorldTick(AngelScriptManager* manager);
void AngelScriptManager_ReleaseEngine(AngelScriptManager* manager);

void AngelScriptManager_CallOnPlayerConnect(AngelScriptManager* manager, struct Client* pClient);
void AngelScriptManager_CallOnPlayerDisconnect(AngelScriptManager* manager, struct Client* pClient);
void AngelScriptManager_CallOnPlayerInput(AngelScriptManager* manager, struct Client* pClient,
					const char* input);


int ASContextPool_Init(ASContextPool* pPool, void* pEngine, size_t initial_size);
void* ASContextPool_GetContextAt(ASContextPool* pPool, size_t idx);
void ASContextPool_ReturnContextByIndex(ASContextPool* pPool, size_t idx);
size_t ASContextPool_GetFreeContextIndex(ASContextPool* pPool);
void ASContextPool_Destroy(ASContextPool* pPool);

#endif
