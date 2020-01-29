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

struct CmdDispatchThread;
typedef struct asIScriptObject asIScriptObject;

struct Client
{
	int sock;
	struct EvPkg ev_pkg;
	struct sockaddr_in addr;

	TelnetStream tel_stream;
	ZCompressor zstreams;

	struct timespec connection_time;
	struct timespec last_input_time;

	float cmd_intervals[CLIENT_STOREDCMDINTERVALS]; // average commands per second sent
	unsigned char interval_idx;

	cv_t input_buffer;

	struct hrt_prioq cmd_queue;
	pthread_mutex_t cmd_queue_mtx;

	struct CmdDispatchThread* pCmdDispatcher;
	struct MemoryPool mem_pool;

	asIScriptObject* player_obj;
};

void Client_Disconnect(struct Client* pTarget);
int Client_WriteTo(struct Client* pTarget, const char* buf, size_t len);
void Client_Sendf(struct Client* pTarget, const char* fmt, ...);
struct Client* Client_Create(int sock, struct CmdDispatchThread*);
void Client_Destroy(void* p);

void Client_QueueCommand(struct Client* pClient, void* (*taskfn) (void*),
			time_t runtime_s, long runtime_ns, void* args, void (*argreleaserfn) (void*));

#ifdef __cplusplus
}
#endif

#endif
