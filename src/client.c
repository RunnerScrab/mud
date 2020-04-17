#include "client.h"
#include "talloc.h"
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include "threadpool.h"
#include "zcompressor.h"
#include "iohelper.h"
#include "ansicolor.h"
#include "command_dispatch.h"
#include "as_cinterface.h"

size_t Client_GetRefCount(struct Client* client)
{
	pthread_rwlock_rdlock(&client->refcount_rwlock);
	size_t retval = client->refcount;
	pthread_rwlock_unlock(&client->refcount_rwlock);
	return retval;
}

void Client_AddRef(struct Client* client)
{
	pthread_rwlock_wrlock(&client->refcount_rwlock);
	++client->refcount;
	pthread_rwlock_unlock(&client->refcount_rwlock);
}

void Client_ReleaseRef(struct Client* client)
{
	pthread_rwlock_wrlock(&client->refcount_rwlock);
	--client->refcount;
	pthread_rwlock_unlock(&client->refcount_rwlock);
}

struct Client* Client_Create(int sock, struct Server* server, struct CmdDispatchThread* pDispatcher)
{
	struct Client* pClient = (struct Client*) talloc(sizeof(struct Client));
	memset(pClient, 0, sizeof(struct Client));

	pClient->server = server;

	pClient->player_obj = 0;
	pClient->pCmdDispatcher = pDispatcher;
	memset(&pClient->tel_stream, 0, sizeof(TelnetStream));

	pthread_mutex_init(&pClient->connection_state_mtx, 0);
	ZCompressor_Init(&pClient->zstreams);
	cv_init(&pClient->tel_stream.sb_args, 32);

	pClient->tel_stream.sock = sock;
	pClient->sock = sock;
	pClient->ev_pkg.sockfd = sock;
	pClient->ev_pkg.pData = pClient;

	clock_gettime(CLOCK_BOOTTIME, &(pClient->connection_time));
	clock_gettime(CLOCK_BOOTTIME, &(pClient->last_input_time));
	pClient->interval_idx = 0;

	cv_init(&pClient->input_buffer, CLIENT_MAXINPUTLEN);
	memset(pClient->cmd_intervals, 0, sizeof(float) * 6); //This is for command rate stat tracking

	hrt_prioq_create(&pClient->cmd_queue, 32);
	pthread_mutex_init(&pClient->cmd_queue_mtx, 0);
	MemoryPool_Init(&pClient->mem_pool);

	pthread_rwlock_init(&pClient->refcount_rwlock, 0);
	return pClient;
}

void Client_Disconnect(struct Client* pClient)
{
	pClient->bDisconnected = 1;
	close(pClient->sock);
}

void Client_Destroy(void* p)
{
	struct Client* pClient = (struct Client*) p;
	pthread_mutex_lock(&pClient->connection_state_mtx);
	cv_destroy(&pClient->tel_stream.sb_args);
	cv_destroy(&pClient->input_buffer);
	ZCompressor_StopAndRelease(&pClient->zstreams);

	pthread_mutex_destroy(&pClient->connection_state_mtx);
	pthread_mutex_lock(&pClient->cmd_queue_mtx);
	hrt_prioq_destroy(&pClient->cmd_queue);
	pthread_mutex_destroy(&pClient->cmd_queue_mtx);
	MemoryPool_Destroy(&pClient->mem_pool);

	asIScriptObject_Release(&pClient->player_obj);
	pthread_rwlock_destroy(&pClient->refcount_rwlock);
	tfree(pClient);
}

//This should be used for all client writes, since it knows whether or not
//to perform stream compression
int Client_WriteTo(struct Client* pTarget, const char* buf, size_t len)
{
	cv_t color_buf;
	cv_init(&color_buf, len);
	ANSIColorizeString(buf, len, &color_buf);

	pthread_mutex_lock(&pTarget->connection_state_mtx);
	switch(pTarget->tel_stream.opts.b_mccp2)
	{
	case 1:
	{
		cv_t compout;
		cv_init(&compout, len);

		//We don't need or want to send the null terminator over the network
		if(ZCompressor_CompressRawData(&pTarget->zstreams,
						color_buf.data, color_buf.length - 1, &compout) < 0)
		{
			pthread_mutex_unlock(&pTarget->connection_state_mtx);
			cv_destroy(&compout);
			cv_destroy(&color_buf);
			return -1;
		}

		int result = write_from_cv_raw(pTarget->sock, &compout);
		pthread_mutex_unlock(&pTarget->connection_state_mtx);
		cv_destroy(&compout);
		cv_destroy(&color_buf);
		return result;
	}
	default:
	{
		//Don't send the null terminator
		size_t written = write_full_raw(pTarget->sock,
						color_buf.data, color_buf.length - 1);
		cv_destroy(&color_buf);
		pthread_mutex_unlock(&pTarget->connection_state_mtx);
		return written;
	}
	}
}

//This uses Client_WriteTo for a printf style send
void Client_Sendf(struct Client* pTarget, const char* fmt, ...)
{
	va_list arglist, argcpy;
	va_start(arglist, fmt);
	va_copy(argcpy, arglist);
	cv_t buf;
	cv_init(&buf, 512);
	size_t required = vsnprintf(buf.data, 512, fmt, arglist) + 1;
	cv_resize(&buf, required);

	buf.length = required;

	Client_WriteTo(pTarget, buf.data, buf.length);

	cv_destroy(&buf);
	va_end(arglist);
	va_end(argcpy);
}


void Client_QueueCommand(struct Client* pClient, void* (*taskfn) (void*),
			time_t runtime_s, long runtime_ns, void* args, void (*argreleaserfn) (void*))
{
	struct ThreadTask* pTask = (struct ThreadTask*) MemoryPool_Alloc(&pClient->mem_pool, sizeof(struct ThreadTask));
	pTask->taskfn = taskfn;
	pTask->pArgs = args;
	pTask->releasefn = argreleaserfn;

	struct timespec ts;
	ts.tv_sec = runtime_s;
	ts.tv_nsec = runtime_ns;

	pthread_mutex_lock(&pClient->cmd_queue_mtx);
	hrt_prioq_min_insert(&pClient->cmd_queue, &ts, pTask);
	pthread_mutex_unlock(&pClient->cmd_queue_mtx);

	//The command dispatch thread will wait on this condition variable if if
	//ever wakes up and finds it has no commands at all (which is going to
	//most of the time - users would be hard pressed to continuously
	//saturate the queue without getting booted for command spam).  When the
	//command dispatch thread does have commands queued, it will instead
	//calculate how long it is before the earliest command must be run, then
	//sleep for the duration.  We need to wake it up if it is sleeping here

	pthread_cond_signal(&pClient->pCmdDispatcher->wakecond);
}
