#ifndef NATIVEACTOR_H_
#define NATIVEACTOR_H_
#include <atomic>
#include <pthread.h>
#include <time.h>
#include "hrt_prioq.h"
#include "poolalloc.h"
#include "threadpool.h"
#include "as_manager.h"
#include "as_refcountedobj.h"

struct ActionScheduler;
class asIScriptEngine;

//The application-backed portion of the script Actor class

class NativeActor : public AS_RefCountedProxiedObj
{
	struct hrt_prioq m_action_queue;
	pthread_mutex_t m_action_queue_mtx;

	struct MemoryPool m_mem_pool;
	static ActionScheduler* sm_pActionScheduler;
	static AngelScriptManager* sm_pAngelScriptManager;
public:
	static void SetAngelScriptManager(AngelScriptManager* manager)
	{
		sm_pAngelScriptManager = manager;
	}

	static AngelScriptManager* GetAngelScriptManager()
	{
		return sm_pAngelScriptManager;
	}

	static void SetActionScheduler(ActionScheduler* as);
	static ActionScheduler* GetActionScheduler()
	{
		return sm_pActionScheduler;
	}

	static NativeActor* Factory();

	void QueueAction(void* (*taskfn)(void*),
			time_t runtime_s, long runtime_ns, void *args,
			void (*argreleaserfn)(void*));

	void QueueScriptAction(asIScriptObject* obj, unsigned int delay_s,
			unsigned int delay_ns);

	void LockQueue()
	{
		pthread_mutex_lock(&m_action_queue_mtx);
	}

	struct hrt_prioq* GetQueue()
	{
		return &m_action_queue;
	}

	void FreeQueueTask(struct ThreadTask* task)
	{
		MemoryPool_Free(&m_mem_pool, sizeof(struct ThreadTask), task);
	}

	MemoryPool* GetMemoryPool()
	{
		return &m_mem_pool;
	}

	void UnlockQueue()
	{
		pthread_mutex_unlock(&m_action_queue_mtx);
	}

	NativeActor(asIScriptObject* obj);
	~NativeActor();
};

int RegisterNativeActorClass(asIScriptEngine* engine, AngelScriptManager* manager);
int LoadActorProxyScript(asIScriptModule* module);

#endif
