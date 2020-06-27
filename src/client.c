#include "client.h"
#include "talloc.h"
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>

#include "utils.h"
#include "threadpool.h"
#include "zcompressor.h"
#include "iohelper.h"
#include "ansicolor.h"
#include "as_cinterface.h"

size_t Client_GetRefCount(struct Client *client)
{
	pthread_rwlock_rdlock(&client->refcount_rwlock);
	size_t retval = client->refcount;
	pthread_rwlock_unlock(&client->refcount_rwlock);
	return retval;
}

void Client_AddRef(struct Client *client)
{
	pthread_rwlock_wrlock(&client->refcount_rwlock);
	++client->refcount;
	pthread_rwlock_unlock(&client->refcount_rwlock);
}

void Client_ReleaseRef(struct Client *client)
{
	pthread_rwlock_wrlock(&client->refcount_rwlock);
	--client->refcount;
	pthread_rwlock_unlock(&client->refcount_rwlock);
}

struct Client* Client_Create(int sock, struct Server *server,
		struct ActionScheduler *pDispatcher)
{
	struct Client *pClient = (struct Client*) talloc(sizeof(struct Client));
	memset(pClient, 0, sizeof(struct Client));

	pClient->server = server;

	pClient->player_obj = 0;
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

	pthread_rwlock_init(&pClient->refcount_rwlock, 0);
	return pClient;
}

void Client_Disconnect(struct Client *pClient)
{
	pClient->bDisconnected = 1;
	close(pClient->sock);
}

void Client_Destroy(void *p)
{
	struct Client *pClient = (struct Client*) p;
	pthread_mutex_lock(&pClient->connection_state_mtx);
	cv_destroy(&pClient->tel_stream.sb_args);
	cv_destroy(&pClient->input_buffer);
	ZCompressor_StopAndRelease(&pClient->zstreams);

	pthread_mutex_destroy(&pClient->connection_state_mtx);

	PlayerConnection_Release(&pClient->player_obj);

	pthread_rwlock_destroy(&pClient->refcount_rwlock);
	tfree(pClient);
}

//This should be used for all client writes, since it knows whether or not
//to perform stream compression
int Client_WriteTo(struct Client *pTarget, const char *buf, size_t len)
{
	if(pTarget->bDisconnected)
	{
		return -1;
	}

	cv_t color_buf;
	cv_init(&color_buf, len);
	ANSIColorizeString(buf, len, &color_buf);

	pthread_mutex_lock(&pTarget->connection_state_mtx);
	switch (pTarget->tel_stream.opts.b_mccp2)
	{
	case 1:
	{
		cv_t compout;
		cv_init(&compout, len);

		//We don't need or want to send the null terminator over the network
		if (ZCompressor_CompressRawData(&pTarget->zstreams, color_buf.data,
				color_buf.length - 1, &compout) < 0)
		{
			pthread_mutex_unlock(&pTarget->connection_state_mtx);
			cv_destroy(&compout);
			cv_destroy(&color_buf);
			return -1;
		}

		int result = write_from_cv_raw(pTarget->sock, &compout);
		if(result < 0)
		{
			int err = errno;
			char msg[512] = {0};
			strerror_r(err, msg, 512);
			dbgprintf("Socket write error: %s\n", msg);
		}
		pthread_mutex_unlock(&pTarget->connection_state_mtx);
		cv_destroy(&compout);
		cv_destroy(&color_buf);
		return result;
	}
	default:
	{
		//Don't send the null terminator
		int written = write_full_raw(pTarget->sock, color_buf.data,
				color_buf.length - 1);
		if(written < 0)
		{
			int err = errno;
			char msg[512] = {0};
			strerror_r(err, msg, 512);
			dbgprintf("Socket write error: %s\n", msg);
		}
		cv_destroy(&color_buf);
		pthread_mutex_unlock(&pTarget->connection_state_mtx);
		return written;
	}
	}
}

//This uses Client_WriteTo for a printf style send
void Client_Sendf(struct Client *pTarget, const char *fmt, ...)
{
	if(!pTarget)
	{
		return;
	}

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
