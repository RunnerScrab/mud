#ifndef THREADPOOL_H_
#define THREADPOOL_H_
#include <pthread.h>
#include <sys/sysinfo.h>
#include "poolalloc.h"
#include "prioq.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ThreadTask
{
	void* (*taskfn)(void*);
	void *pArgs;

	//If releasefn is supplied, thread will call
	//it on the ThreadTask instance to release
	void (*releasefn)(void*);
};

struct ThreadPool
{
	pthread_t *pThreads;
	unsigned int thread_count;

	struct prioq prio_queue;
	pthread_mutex_t prio_queue_mutex;
	pthread_cond_t wakecond;

	volatile unsigned char bIsRunning;

	struct AllocPool alloc_pool;
};

void ThreadPool_Stop(struct ThreadPool *tp);
void ThreadPool_Destroy(struct ThreadPool *tp);
int ThreadPool_Init(struct ThreadPool *tp, unsigned int cores);

//args should be a pointer to allocated memory; the threadpool takes ownership
int ThreadPool_AddTask(struct ThreadPool *tp, void* (*task)(void*),
		int priority, void *args, void (*argreleaserfn)(void*));

#ifdef __cplusplus
}
#endif

#endif
