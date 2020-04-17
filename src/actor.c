#include "actor.h"

#include <pthread.h>
#include <time.h>
#include "threadpool.h"
#include "command_dispatch.h"
#include "as_cinterface.h"

void Actor_Init(struct Actor* pActor, struct CmdDispatchThread* dispatcher)
{
	pActor->refcount = 1;
	pthread_rwlock_init(&pActor->refcount_rwlock, 0);

	hrt_prioq_create(&pActor->action_queue, 32);
	pthread_mutex_init(&pActor->action_queue_mutex, 0);
	MemoryPool_Init(&pActor->mem_pool);
	pActor->pActionDispatcher = dispatcher;
}

void Actor_Destroy(struct Actor* pActor)
{
	pthread_rwlock_wrlock(&pActor->refcount_rwlock);
	pthread_rwlock_destroy(&pActor->refcount_rwlock);
	pthread_mutex_lock(&pActor->action_queue_mutex);
	hrt_prioq_destroy(&pActor->action_queue);
	pthread_mutex_destroy(&pActor->action_queue_mutex);
	MemoryPool_Destroy(&pActor->mem_pool);
}

int Actor_GetRefCount(struct Actor* pActor)
{
	pthread_rwlock_rdlock(&pActor->refcount_rwlock);
	int retval = pActor->refcount;
	pthread_rwlock_unlock(&pActor->refcount_rwlock);
	return retval;
}

void Actor_AddRef(struct Actor* pActor)
{
	pthread_rwlock_wrlock(&pActor->refcount_rwlock);
	++(pActor->refcount);
	pthread_rwlock_unlock(&pActor->refcount_rwlock);
}

void Actor_ReleaseRef(struct Actor* pActor)
{
	pthread_rwlock_wrlock(&pActor->refcount_rwlock);
	--(pActor->refcount);
	pthread_rwlock_unlock(&pActor->refcount_rwlock);
}

void Actor_QueueAction(struct Actor* pActor,
		void* (*taskfn) (void*),
		time_t runtime_s,
		long runtime_ns,
		void* args,
		void (*argreleaserfn) (void*))
{
	struct ThreadTask* pTask = (struct ThreadTask*) MemoryPool_Alloc(&pActor->mem_pool,
									sizeof(struct ThreadTask));
	pTask->taskfn = taskfn;
	pTask->pArgs = args;
	pTask->releasefn = argreleaserfn;

	struct timespec ts;
	ts.tv_sec = runtime_s;
	ts.tv_nsec = runtime_ns;

	pthread_mutex_lock(&pActor->action_queue_mutex);
	hrt_prioq_min_insert(&pActor->action_queue, &ts, pTask);
	pthread_mutex_unlock(&pActor->action_queue_mutex);


	//The command dispatch thread will wait on this condition variable if if
	//ever wakes up and finds it has no commands at all (which is going to
	//most of the time - users would be hard pressed to continuously
	//saturate the queue without getting booted for command spam).  When the
	//command dispatch thread does have commands queued, it will instead
	//calculate how long it is before the earliest command must be run, then
	//sleep for the duration.  We need to wake it up if it is sleeping here

	pthread_cond_signal(&pActor->pActionDispatcher->wakecond);
}
