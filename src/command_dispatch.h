#ifndef COMMAND_DISPATCH_H_
#define COMMAND_DISPATCH_H_
#include <pthread.h>

struct Server;

struct CmdDispatchThreadPkg
{
	struct Server* pServer;
	volatile char bIsRunning;
	pthread_cond_t wakecond;
	pthread_mutex_t wakecondmtx;
};

struct CmdDispatchThread
{
	struct CmdDispatchThreadPkg thread_pkg;
	pthread_t thread;
};

void CmdDispatchThread_Init(struct CmdDispatchThread* thread, struct Server* server);
void CmdDispatchThread_Stop(struct CmdDispatchThread* dispatchthread);
void CmdDispatchThread_Destroy(struct CmdDispatchThread* thread);

void* UserCommandDispatchThreadFn(void* pArgs);

#endif
