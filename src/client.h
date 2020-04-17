#ifndef CLIENT_H_
#define CLIENT_H_
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <time.h>
#include "telnet.h"
#include "constants.h"
#include "charvector.h"
#include "zcompressor.h"
#include "hrt_prioq.h"
#include "poolalloc.h"

#ifdef __cplusplus
extern "C" {
#endif

	struct EvPkg
	{
		int sockfd;
		void* pData;
	};

	struct Server;
	struct CmdDispatchThread;
	typedef struct asIScriptObject asIScriptObject;
	struct Actor;

	struct Client
	{
		int sock;
		struct EvPkg ev_pkg;
		struct sockaddr_in addr;

		TelnetStream tel_stream;
		ZCompressor zstreams;
		pthread_mutex_t connection_state_mtx;

		struct timespec connection_time;
		struct timespec last_input_time;

		unsigned int bytes_sent_at_intervals[CLIENT_STOREDCMDINTERVALS];
		float cmd_intervals[CLIENT_STOREDCMDINTERVALS]; // average commands per second sent
		unsigned char interval_idx;

		cv_t input_buffer;

		struct hrt_prioq cmd_queue;
		pthread_mutex_t cmd_queue_mtx;

		struct Server* server;

		struct CmdDispatchThread* pCmdDispatcher;
		struct MemoryPool mem_pool;

		asIScriptObject* player_obj;

		unsigned char bDisconnected;
		unsigned int refcount;
		pthread_rwlock_t refcount_rwlock;

	};

	void Client_Disconnect(struct Client* pTarget);
	int Client_WriteTo(struct Client* pTarget, const char* buf, size_t len); //What the rest of the engine should use for client output
	void Client_Sendf(struct Client* pTarget, const char* fmt, ...); //Uses WriteTo
	struct Client* Client_Create(int sock, struct Server*, struct CmdDispatchThread*);
	void Client_Destroy(void* p);

	size_t Client_GetRefCount(struct Client* client);
	void Client_ReleaseRef(struct Client* client);
	void Client_AddRef(struct Client* client);

	void Client_QueueCommand(struct Client* pClient, void* (*taskfn) (void*),
				time_t runtime_s, long runtime_ns, void* args, void (*argreleaserfn) (void*));

#ifdef __cplusplus
}
#endif

#endif
