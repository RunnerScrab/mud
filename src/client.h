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
extern "C"
{
#endif

struct EvPkg
{
	int sockfd;
	void *pData;
};

struct Server;
struct ActionScheduler;
typedef struct asIScriptObject asIScriptObject;
struct Actor;
typedef struct PlayerConnection PlayerConnection;

struct Client
{
	int sock;
	struct EvPkg ev_pkg;
	struct sockaddr_in addr;
	char ip_address_text[64];

	TelnetStream tel_stream;
	ZCompressor zstreams;
	pthread_mutex_t connection_state_mtx;

	struct timespec connection_time;
	struct timespec last_input_time;

	unsigned int bytes_sent_at_intervals[CLIENT_STOREDCMDINTERVALS];
	float cmd_intervals[CLIENT_STOREDCMDINTERVALS]; // average commands per second sent
	unsigned char interval_idx;

	cv_t input_buffer;

	struct Server *server;

	struct MemoryPool mem_pool;

	PlayerConnection *player_obj;

	unsigned char bDisconnected;
	unsigned int refcount;
	pthread_rwlock_t refcount_rwlock;

};

void Client_Disconnect(struct Client *pTarget);
int Client_WriteTo(struct Client *pTarget, const char *buf, size_t len); //What the rest of the engine should use for client output
void Client_Sendf(struct Client *pTarget, const char *fmt, ...); //Uses WriteTo
struct Client* Client_Create(int sock, struct Server*, struct ActionScheduler*);
void Client_Destroy(void *p);

size_t Client_GetRefCount(struct Client *client);
void Client_ReleaseRef(struct Client *client);
void Client_AddRef(struct Client *client);


#ifdef __cplusplus
}
#endif

#endif
