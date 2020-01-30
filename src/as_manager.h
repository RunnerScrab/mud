#ifndef ANGELSCRIPT_MANAGER_H
#define ANGELSCRIPT_MANAGER_H
#ifdef __cplusplus
extern "C"
{
#include "poolalloc.h"
}
#else
#include "poolalloc.h"
#endif

#include <pthread.h>

#ifdef __cplusplus
class asIScriptContext;
class asIScriptEngine;
class asCJITCompiler;
class asIScriptModule;
class asIScriptFunction;
#else
typedef struct asIScriptContext asIScriptContext;
typedef struct asIScriptEngine asIScriptEngine;
typedef struct asCJITCompiler asCJITCompiler;
typedef struct asIScriptModule asIScriptModule;
typedef struct asIScriptFunction asIScriptFunction;
#endif

struct Server;
struct Client;
struct ServerConfig;

typedef struct
{
	asIScriptContext** pContextArray;
	unsigned char* pInUseArray;

	size_t ctx_count;

	asIScriptEngine* pEngine;

	pthread_mutex_t mtx;
} ASContextPool;

typedef struct
{
	asIScriptEngine* engine;
	asCJITCompiler* jit;

	asIScriptModule* main_module, *config_module;

	asIScriptFunction* world_tick_func, *on_player_connect_func;
	asIScriptFunction* on_player_disconnect_func, *on_player_input_func;
	asIScriptFunction* server_setup_func;
	asIScriptContext* world_tick_scriptcontext;

	struct MemoryPool mem_pool;
	ASContextPool ctx_pool;

	size_t next_free_context_idx;
}
	AngelScriptManager;

#ifdef __cplusplus
extern "C"
{
#endif
	extern int AngelScriptManager_InitEngine(AngelScriptManager* manager);
	extern int AngelScriptManager_LoadServerConfig(AngelScriptManager* manager, struct ServerConfig* server);
	extern int AngelScriptManager_LoadScripts(AngelScriptManager* manager, const char* script_dir);
	extern int AngelScriptManager_InitAPI(AngelScriptManager* manager, struct Server* server);
	extern void AngelScriptManager_RunWorldTick(AngelScriptManager* manager);
	extern void AngelScriptManager_ReleaseEngine(AngelScriptManager* manager);

	extern void AngelScriptManager_CallOnPlayerConnect(AngelScriptManager* manager, struct Client* pClient);
	extern void AngelScriptManager_CallOnPlayerDisconnect(AngelScriptManager* manager, struct Client* pClient);
	extern void AngelScriptManager_CallOnPlayerInput(AngelScriptManager* manager, struct Client* pClient,
						const char* input);


	extern int ASContextPool_Init(ASContextPool*, asIScriptEngine*, AngelScriptManager*, size_t);
	extern asIScriptContext* ASContextPool_GetContextAt(ASContextPool* pPool, size_t idx);
	extern void ASContextPool_ReturnContextByIndex(ASContextPool* pPool, size_t idx);
	extern size_t ASContextPool_GetFreeContextIndex(ASContextPool* pPool);
	extern void ASContextPool_Destroy(ASContextPool* pPool);

#ifdef __cplusplus
}
#endif

#endif
