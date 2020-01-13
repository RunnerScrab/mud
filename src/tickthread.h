#ifndef TICKTHREAD_H_
#define TICKTHREAD_H_
#include <pthread.h>

struct Server;

struct TickThreadPkg
{
	struct Server* pServer;
	volatile char bIsRunning;
	size_t tick_delay;
};

struct TickThread
{
	struct TickThreadPkg thread_pkg;
	pthread_t thread;
};

void TickThread_Init(struct TickThread* tt, struct Server* server, size_t tickspeed);
void TickThread_Stop(struct TickThread* tt);

#endif
