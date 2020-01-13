#include "tickthread.h"
#include "as_manager.h"
#include "prioq.h"
#include "poolalloc.h"
#include "server.h"
#include <stddef.h>
#include <pthread.h>
#include <angelscript.h>

void* TickThreadFn(void* pArgs);

void TickThread_Init(struct TickThread* tt, struct Server* server, size_t tickspeed)
{
	tt->thread_pkg.pServer = server;
	tt->thread_pkg.bIsRunning = 1;
	tt->thread_pkg.tick_delay = tickspeed;
	pthread_create(&tt->thread, 0, TickThreadFn, (void*) &tt->thread_pkg);
}

void TickThread_Stop(struct TickThread* tt)
{
	tt->thread_pkg.bIsRunning = 0;
	pthread_join(tt->thread, 0);
}

void* TickThreadFn(void* pArgs)
{
	//The idea here is that timed events are allocated from the
	//server's pool allocator and enqueued on their own priority
	//queue keyed by the time they are to execute.

	//In addition to conventional tick duties, this thread
	//checks the queue of timed events and sees which are ready
	//to execute. It dispatches those events which are ready to
	//the server's threadpool and does not run them itself.

	//The importance of this is that work to be done on a server
	//tick may occur simultaneously.

	struct TickThreadPkg* pPkg = (struct TickThreadPkg*) pArgs;
	struct Server* pServer = pPkg->pServer;
	const size_t tick_delay = pPkg->tick_delay;
	time_t curtime;

	while(pPkg->bIsRunning)
	{
		Server_SendAllClients(pServer, "Tick!\r\n\r\n");
		curtime = time(0);

		AngelScriptManager_RunWorldTick(&pServer->as_manager);

		//Dispatch ready server events
		pthread_mutex_lock(&pServer->timed_queue_mtx);
		while(prioq_get_size(&pServer->timed_queue) > 0 &&
			curtime >=
			prioq_get_key_at(&pServer->timed_queue, 0))
		{
			struct ThreadTask* pTask = (struct ThreadTask*) prioq_extract_min(&pServer->timed_queue);
			ThreadPool_AddTask(&pServer->thread_pool,
					pTask->taskfn, 0,
					pTask->pArgs, pTask->releasefn);
			MemoryPool_Free(&pServer->mem_pool, sizeof(struct ThreadTask), pTask);
		}
		pthread_mutex_unlock(&pServer->timed_queue_mtx);

		usleep(tick_delay);
	}
	ServerLog(SERVERLOG_STATUS, "Tickthread terminating.\n");
	asThreadCleanup();
	return (void*) 0;
}
