#ifndef CLIENT_H_
#define CLIENT_H_
#include <sys/socket.h>
#include <stdarg.h>
#include <time.h>
#include "telnet.h"
#include "constants.h"
#include "charvector.h"
#include "zcompressor.h"
#include "prioq.h"
#include "poolalloc.h"

struct EvPkg
{
	int sockfd;
	void* pData;
};

struct CmdDispatchThread;

struct Client
{
	int sock;
	struct EvPkg ev_pkg;
	struct sockaddr addr;

	TelnetStream tel_stream;
	ZCompressor zstreams;

	struct timespec connection_time;
	struct timespec last_input_time;

	float cmd_intervals[CLIENT_STOREDCMDINTERVALS]; // average commands per second sent
	unsigned char interval_idx;

	cv_t input_buffer;

	struct prioq cmd_queue;
	pthread_mutex_t cmd_queue_mtx;

	struct CmdDispatchThread* pCmdDispatcher;
	struct MemoryPool mem_pool;
};


int Client_WriteTo(struct Client* pTarget, const char* buf, size_t len);
void Client_Sendf(struct Client* pTarget, const char* fmt, ...);
struct Client* Client_Create(int sock, struct CmdDispatchThread*);
void Client_Destroy(void* p);

void Client_QueueCommand(struct Client* pClient, void* (*taskfn) (void*),
			time_t runtime, void* args, void (*argreleaserfn) (void*));


#endif
