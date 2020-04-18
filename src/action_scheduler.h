#ifndef ACTION_SCHEDULER_H_
#define ACTION_SCHEDULER_H_
#include <set>
#include <atomic>
#include <pthread.h>

#include "nativeactor.h"

struct Server;

class ActionScheduler
{
	pthread_t m_thread;
	struct Server *m_pServer;
	std::atomic<char> m_bIsRunning;
	pthread_cond_t m_wakecond;
	pthread_condattr_t m_wakecondattr;
	pthread_mutex_t m_wakecondmtx;

	std::set<NativeActor*> m_active_actors;
	pthread_rwlock_t m_active_actors_rwlock;

	static void* ThreadFunction(void* pArgs);

public:
	ActionScheduler(struct Server* server);
	void StartThread();
	void StopThread();
	void Signal()
	{
		pthread_cond_signal(&m_wakecond);
	}

	~ActionScheduler();

	void AddActiveActor(NativeActor*);
	void RemoveActor(NativeActor*);
};

void* ActionSchedulerThreadFunction(void *pArgs);

#endif
