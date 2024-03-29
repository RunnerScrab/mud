#ifndef SERVER_H_
#define SERVER_H_
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <pthread.h>

#include "client.h"
#include "vector.h"
#include "threadpool.h"
#include "constants.h"
#include "prioq.h"
#include "as_manager.h"
#include "database.h"

#include "serverconfig.h"
#include "server_init.h"

#include "rand.h"
#include "crypto.h"
#include "tickthread.h"

#define SUCCESS(x) (x >= 0)
#define FAILURE(x) (x < 0)

extern const char *g_ServerLogTypes[];
extern const int SERVERLOG_DEBUG;
extern const int SERVERLOG_STATUS;
extern const int SERVERLOG_ERROR;

struct Server
{
	int sockfd;
	int epfd;
	int cmd_pipe[2]; //Used for commands to the dispatch thread

	struct ServerConfig configuration;
	struct EvPkg epkg; //Used for epoll to pass data back to us via epoll_event
	struct epoll_event server_event, cmdpipe_event;
	struct epoll_event *evlist;
	size_t evlist_len;
	struct sockaddr_in addr_in;
	struct Vector clients;
	pthread_rwlock_t clients_rwlock;

	struct ThreadPool thread_pool;
	unsigned int cpu_cores;

	struct MemoryPool mem_pool;

	struct prioq timed_queue;
	pthread_mutex_t timed_queue_mtx;

	struct TickThread game_tick_thread;

	struct RandGenerator rand_generator;

	AngelScriptManager as_manager; //angelscript engine manager
	struct CryptoManager crypto_manager;
	struct Database db;

	char *MOTD;
	size_t MOTDlen;

	struct timespec realtime_offset; //The CLOCK_MONOTONIC to CLOCK_REALTIME offset from server start
};

void ServerLog(unsigned int code, const char *fmt, ...);

int CompClientSock(void *key, void *p);

void Server_WriteToCmdPipe(struct Server *server, const char *msg,
		size_t msglen);

int Server_Teardown(struct Server *pServer);

int Server_AcceptClient(struct Server *server);
void Server_HandleUserInput(struct Server *pServer, struct Client *pClient);

void Server_HandleClientDisconnect(struct Server *pServer,
		struct Client *pClient);
void Server_SendAllClients(struct Server *pServer, const char *fmt, ...);
void Server_AddTimedTask(struct Server *pServer, void* (*taskfn)(void*),
		time_t runtime, void *args, void (*argreleaserfn)(void*));
#endif
