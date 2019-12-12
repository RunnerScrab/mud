#ifndef THREADPOOL_H_
#define THREADPOOL_H_
#include <pthread.h>
#include <sys/sysinfo.h>
#include "poolalloc.h"
#include "heap.h"

struct ThreadTask
{
	void* (*taskfn) (void*);
	void* pArgs;

	//If releasefn is supplied, thread will call
	//it on the ThreadTask instance to release
	void (*releasefn) (void*);
};


struct ThreadBundle
{
	unsigned int thread_num; //Not the Pid, but a number < than the # of cpu cores
	struct ThreadPool* thread_pool;
	struct Heap my_pq;
	pthread_mutex_t my_pq_mtx;
	pthread_cond_t my_wakecond;
	volatile unsigned char bShouldBeRunning;
	struct MemoryPool mem_pool;
	pthread_mutex_t mem_pool_mtx;
};


int ThreadBundle_Init(struct ThreadBundle* tb, struct ThreadPool* tp,
		unsigned int thread_num);
void ThreadBundle_Destroy(struct ThreadBundle* tb);


struct ThreadPool
{
	struct ThreadBundle* thread_bundles;
	pthread_t* pThreads;
	unsigned int thread_count, last_thread_assigned;
};

struct ThreadBundle* ThreadPool_GetLeastBusyThread(struct ThreadPool* tp);

void ThreadPool_Stop(struct ThreadPool* tp);
void ThreadPool_Destroy(struct ThreadPool* tp);
int ThreadPool_Init(struct ThreadPool* tp, unsigned int cores);

int ThreadPool_AddTask(struct ThreadPool* tp, struct ThreadBundle* tb,
		void* (*task) (void*), int priority, void* args,
		void (*argreleaserfn) (void*));

#endif
