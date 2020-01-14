#include "command_dispatch.h"
#include "server.h"

void CmdDispatchThreadPkg_Init(struct CmdDispatchThreadPkg* pkg, struct Server* server)
{
	pkg->pServer = server;
	pkg->bIsRunning = 1;
	pthread_cond_init(&pkg->wakecond, 0);
	pthread_mutex_init(&pkg->wakecondmtx, 0);
}

void CmdDispatchThreadPkg_Destroy(struct CmdDispatchThreadPkg* pkg)
{
	pthread_cond_destroy(&pkg->wakecond);
	pthread_mutex_destroy(&pkg->wakecondmtx);
}

void CmdDispatchThread_Init(struct CmdDispatchThread* dispatchthread, struct Server* server)
{
	CmdDispatchThreadPkg_Init(&dispatchthread->thread_pkg, server);
	pthread_create(&dispatchthread->thread, 0, UserCommandDispatchThreadFn,
		(void*) &dispatchthread->thread_pkg);

}

void CmdDispatchThread_Stop(struct CmdDispatchThread* dispatchthread)
{
	dispatchthread->thread_pkg.bIsRunning = 0;
	pthread_cond_broadcast(&dispatchthread->thread_pkg.wakecond);
	pthread_join(dispatchthread->thread, 0);
}

void CmdDispatchThread_Destroy(struct CmdDispatchThread* dispatchthread)
{
	CmdDispatchThreadPkg_Destroy(&dispatchthread->thread_pkg);
}


void* UserCommandDispatchThreadFn(void* pArgs)
{
	//The reason user command dispatch shouldn't be in the tick
	//thread is that command dispatch should not be limited to every server tick
	ServerLog(SERVERLOG_STATUS, "Running user command dispatch thread\n");
	struct CmdDispatchThreadPkg* pPkg = (struct CmdDispatchThreadPkg*) pArgs;
	struct Server* pServer = pPkg->pServer;
	time_t curtime;
	size_t idx = 0, len = 0;
	time_t min_delay = 0;
	while(pPkg->bIsRunning)
	{
		curtime = time(0);
		pthread_mutex_lock(&pServer->clients_mtx);

		for(idx = 0, len = Vector_Count(&pServer->clients);
		    idx < len; ++idx)
		{
			struct Client* pClient = (struct Client*) Vector_At(&pServer->clients, idx);
			pthread_mutex_lock(&pClient->cmd_queue_mtx);
			time_t delay = 0;
			while(prioq_get_size(&pClient->cmd_queue) > 0 &&
				curtime >= (delay = prioq_get_key_at(&pClient->cmd_queue, 0)))
			{
				struct ThreadTask* pTask = (struct ThreadTask*) prioq_extract_min(&pClient->cmd_queue);
				ThreadPool_AddTask(&pServer->thread_pool, pTask->taskfn, 0,
						pTask->pArgs, pTask->releasefn);
				MemoryPool_Free(&pClient->mem_pool, sizeof(struct ThreadTask), pTask);
			}

			pthread_mutex_unlock(&pClient->cmd_queue_mtx);
			if(delay > curtime)
			{
				min_delay = min_delay > delay ? delay : min_delay;
			}

		}

		pthread_mutex_unlock(&pServer->clients_mtx);

		if(min_delay > curtime)
		{
			sleep(curtime - min_delay);
		}
		else
		{
			pthread_cond_wait(&pPkg->wakecond, &pPkg->wakecondmtx);
			pthread_mutex_unlock(&pPkg->wakecondmtx);
		}

	}

	asThreadCleanup();
	return (void*) 0;
}
