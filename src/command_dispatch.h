#ifndef COMMAND_DISPATCH_H_
#define COMMAND_DISPATCH_H_
#include <pthread.h>
#include "vector.h"
#include "actor.h"

struct Server;

struct CmdDispatchThread
{
	pthread_t thread;
	struct Server* pServer;
	volatile char bIsRunning;
	pthread_cond_t wakecond;
	pthread_condattr_t wakecondattr;
	pthread_mutex_t wakecondmtx;

	struct Vector active_actors;
	pthread_mutex_t active_actors_mtx;
};

int CmdDispatchThread_Init(struct CmdDispatchThread* thread, struct Server* server);
void CmdDispatchThread_Stop(struct CmdDispatchThread* dispatchthread);
void CmdDispatchThread_Destroy(struct CmdDispatchThread* thread);

void* UserCommandDispatchThreadFn(void* pArgs);

#endif
