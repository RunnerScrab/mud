#ifndef TICKTHREAD_H_
#define TICKTHREAD_H_
#include <pthread.h>

struct Server;

struct TickThread
{
	pthread_t thread;

	struct Server *pServer;
	volatile char bIsRunning;
	size_t tick_delay;

};

int TickThread_Init(struct TickThread *tt, struct Server *server,
		size_t tickspeed);
void TickThread_Stop(struct TickThread *tt);

#endif
