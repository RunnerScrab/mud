#include "nativeactor.h"

#include <string>

#include <pthread.h>
#include <time.h>

#include "utils.h"
#include "angelscript.h"
#include "as_manager.h"
#include "as_api.h"
#include "action_scheduler.h"
#include "threadpool.h"
#include "as_cinterface.h"

ActionScheduler *NativeActor::sm_pActionScheduler = 0;
AngelScriptManager *NativeActor::sm_pAngelScriptManager = 0;

static const char *actorscript = "shared abstract class Actor"
		"{"
		"Actor(){ @m_obj = NativeActor_t();}"
		"void QueueAction(IAction@+ cmd, uint32 delay_s, uint32 delay_ns)"
		"{ m_obj.QueueAction(cmd, delay_s, delay_ns);}"
		"private NativeActor_t@ m_obj;"
		"}";

void NativeActor::SetActionScheduler(ActionScheduler *as)
{
	sm_pActionScheduler = as;
}

int LoadActorProxyScript(asIScriptModule* module)
{
	return module->AddScriptSection("game", actorscript, strlen(actorscript));
}

int RegisterNativeActorClass(asIScriptEngine *engine,
		AngelScriptManager *manager)
{
	NativeActor::SetAngelScriptManager(manager);
	NativeActor::SetActionScheduler(manager->action_scheduler);

	int result = 0;
	result = engine->RegisterObjectType("NativeActor_t", 0, asOBJ_REF);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectBehaviour("NativeActor_t", asBEHAVE_FACTORY,
			"NativeActor_t @f()", asFUNCTION(NativeActor::Factory), asCALL_CDECL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectBehaviour("NativeActor_t", asBEHAVE_ADDREF,
			"void f()", asMETHOD(NativeActor, AddRef), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectBehaviour("NativeActor_t", asBEHAVE_RELEASE,
			"void f()", asMETHOD(NativeActor, Release), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("NativeActor_t",
			"void QueueAction(IAction@+ cmd, uint32 delay_s, uint32 delay_ns)",
			asMETHOD(NativeActor, QueueScriptAction), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	return result;
}

NativeActor* NativeActor::Factory()
{
	asIScriptContext *ctx = asGetActiveContext();
	asIScriptFunction *func = ctx->GetFunction(0);
	if (0 == func->GetObjectType()
			|| std::string(func->GetObjectType()->GetName()) != "Actor")
	{
		ctx->SetException("Invalid attempt to manually instantiate Actor");
	}

	asIScriptObject *obj =
			reinterpret_cast<asIScriptObject*>(ctx->GetThisPointer(0));
	return new NativeActor(obj);
}

NativeActor::NativeActor(asIScriptObject *obj) :
		AS_RefCountedObj(obj)
{
	hrt_prioq_create(&m_action_queue, 32);
	pthread_mutex_init(&m_action_queue_mtx, 0);
	MemoryPool_Init(&m_mem_pool);
}

NativeActor::~NativeActor()
{
	pthread_mutex_lock(&m_action_queue_mtx);
	hrt_prioq_destroy(&m_action_queue);
	pthread_mutex_destroy(&m_action_queue_mtx);
	MemoryPool_Destroy(&m_mem_pool);
}

void NativeActor::QueueScriptAction(asIScriptObject *obj, unsigned int delay_s,
		unsigned int delay_ns)
{
	//Wraps a script action in a threadpool task
	if (obj)
	{
		obj->AddRef();
		struct RunScriptCmdPkg *pkg =
				(struct RunScriptCmdPkg*) MemoryPool_Alloc(&m_mem_pool,
						sizeof(struct RunScriptCmdPkg));
		AngelScriptManager *manager = GetAngelScriptManager();
		pkg->cmd = obj;
		pkg->cmdtype = manager->main_module->GetTypeInfoByDecl("IAction");
		pkg->pMemPool = &m_mem_pool;
		pkg->engine = manager->engine;
		pkg->context_pool = &manager->ctx_pool;

		struct timespec curtime;
		if(clock_gettime(CLOCK_MONOTONIC, &curtime) >= 0)
		{
			QueueAction(ASAPI_RunScriptCommand, curtime.tv_sec + delay_s,
					curtime.tv_nsec + delay_ns, pkg, 0);
		}
	}
}

void NativeActor::QueueAction(void* (*taskfn)(void*), time_t runtime_s,
		long runtime_ns, void *args, void (*argreleaserfn)(void*))
{
	//Queues a threadpool task into the scheduler
	struct ThreadTask *pTask = (struct ThreadTask*) MemoryPool_Alloc(
			&m_mem_pool, sizeof(struct ThreadTask));
	pTask->taskfn = taskfn;
	pTask->pArgs = args;
	pTask->releasefn = argreleaserfn;

	struct timespec ts;
	ts.tv_sec = runtime_s;
	ts.tv_nsec = runtime_ns;

	pthread_mutex_lock(&m_action_queue_mtx);
	hrt_prioq_min_insert(&m_action_queue, &ts, pTask);
	pthread_mutex_unlock(&m_action_queue_mtx);

	//The command dispatch thread will wait on this condition variable if if
	//ever wakes up and finds it has no commands at all (which is going to
	//most of the time - users would be hard pressed to continuously
	//saturate the queue without getting booted for command spam).  When the
	//command dispatch thread does have commands queued, it will instead
	//calculate how long it is before the earliest command must be run, then
	//sleep for the duration.  We need to wake it up if it is sleeping here

	NativeActor::sm_pActionScheduler->Signal();
}
