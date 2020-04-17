#ifndef ACTOR_H_
#define ACTOR_H_

#include <time.h>
#include "hrt_prioq.h"
#include "poolalloc.h"

#ifdef __cplusplus
extern "C" {
#endif


	struct CmdDispatchThread;

	struct Actor
	{
		int refcount;
		pthread_rwlock_t refcount_rwlock;

		struct hrt_prioq action_queue;
		pthread_mutex_t action_queue_mutex;

		struct CmdDispatchThread* pActionDispatcher;
		struct MemoryPool mem_pool;
	};

	void Actor_Init(struct Actor*, struct CmdDispatchThread*);
	void Actor_Destroy(struct Actor*);

	int Actor_GetRefCount(struct Actor*);
	void Actor_AddRef(struct Actor*);
	void Actor_ReleaseRef(struct Actor*);
	void Actor_QueueAction(struct Actor* pActor, void* (*taskfn) (void*), time_t runtime_s, long runtime_ns,
			void* args, void (*argreleaserfn) (void*));
#ifdef __cplusplus
}
#endif

#endif
