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

	struct EvPkg epkg; //Used for epoll to pass data back to us via epoll_event
	struct epoll_event server_event, cmdpipe_event;
	struct epoll_event* evlist;
	size_t evlist_len;
	struct sockaddr_in addr_in;
	struct Vector clients;

	struct ThreadPool thread_pool;
	unsigned int cpu_cores;

	struct MemoryPool mem_pool;

	struct prioq timed_queue;
	pthread_mutex_t timed_queue_mtx;

	AngelScriptManager as_manager; //angelscript engine manager
	char* MOTD;
};

void ServerLog(unsigned int code, const char* fmt, ...);

int CompClientSock(void* key, void* p);

void Server_WriteToCmdPipe(struct Server* server, const char* msg, size_t msglen);

int Server_Configure(struct Server* server, const char* szAddr, unsigned short port);
int Server_Initialize(struct Server* server, unsigned int backlog);
int Server_Teardown(struct Server* pServer);

int Server_AcceptClient(struct Server* server);
void Server_HandleUserInput(struct Server* pServer, struct Client* pClient);

void Server_HandleClientDisconnect(struct Server* pServer, struct Client* pClient);
void Server_SendAllClients(struct Server* pServer, const char* fmt, ...);
#endif

void Server_AddTimedTask(struct Server* pServer, void* (*taskfn) (void*),
			time_t runtime, void* args,
			void (*argreleaserfn) (void*));
