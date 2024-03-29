#include "server.h"
#include "utils.h"
#include <sys/sysinfo.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/epoll.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#include "talloc.h"
#include "vector.h"
#include "client.h"
#include "threadpool.h"
#include "charvector.h"
#include "iohelper.h"
#include "telnet.h"
#include "zcompressor.h"
#include "ansicolor.h"
#include "rand.h"
#include "crypto.h"
#include "mpnumbers.h"

#include "tickthread.h"

#include "as_manager.h"
#include "as_cinterface.h"

static int Server_InitializeADTs(struct Server *server);
static int Server_InitializeScriptEngine(struct Server *server);
static int Server_LoadConfiguration(struct Server *server);
static int Server_LoadGame(struct Server *server);
static int Server_InitializeThreads(struct Server *server);
static int Server_InitializeNetwork(struct Server *server, const char *szAddr,
				unsigned short port);
static void Server_StopNetwork(struct Server *server);
static void Server_StopThreads(struct Server *server);
static void Server_UnloadGame(struct Server *server);
static void Server_FreeConfiguration(struct Server *server);
static void Server_StopScriptEngine(struct Server *server);
static void Server_ReleaseADTs(struct Server *server);

static int Server_LoadMOTD(struct Server *server)
{
	//TODO: Put MOTD filename/path in configuration
	FILE *fp = fopen("motd.txt", "rb");
	if (!fp)
	{
		return -1;
	}
	fseek(fp, 0, SEEK_END);
	size_t len = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	server->MOTD = (char*) talloc(sizeof(char) * (len + 1));
	fread(server->MOTD, sizeof(char), len, fp);
	server->MOTD[len] = 0;
	server->MOTDlen = len;
	fclose(fp);
	return 0;
}

static int Server_InitializeADTs(struct Server *server)
{
	if(FAILURE(MultiPrecisionLibrary_Init()))
	{
		ServerLog(SERVERLOG_ERROR,
			"Failed to initialize multiprecision library.");
		Server_Stop(server);
		return -1;
	}

	pthread_mutex_init(&server->timed_queue_mtx, 0);
	prioq_create(&server->timed_queue, 32);

	pthread_rwlock_init(&server->clients_rwlock, 0);

	pthread_rwlock_wrlock(&server->clients_rwlock);
	if (FAILURE(Vector_Create(&(server->clients), 64, Client_Destroy)))
	{
		ServerLog(SERVERLOG_ERROR,
			"FATAL: Failed to allocate memory for client list!");
		Server_Stop(server);
		return -1;
	}
	pthread_rwlock_unlock(&server->clients_rwlock);

	if (FAILURE(RandGenerator_Init(&server->rand_generator)))
	{
		ServerLog(SERVERLOG_ERROR,
			"FATAL: Failed to initialize random number generator!");
		Server_Stop(server);
		return -1;
	}
	ServerLog(SERVERLOG_STATUS, "Initialized random number generator.");

	if (FAILURE(CryptoManager_Init(&server->crypto_manager)))
	{
		ServerLog(SERVERLOG_ERROR,
			"FATAL: Failed to initialize cryptographic module!");
		Server_Stop(server);
		return -1;
	}
	ServerLog(SERVERLOG_STATUS, "Initialized cryptographic module.");

	return 0;
}

static int Server_InitializeScriptEngine(struct Server *server)
{
	int result = AngelScriptManager_InitEngine(&server->as_manager, server);
	if (FAILURE(result))
	{
		ServerLog(SERVERLOG_ERROR, "Failed to initialize AngelScript engine.");
		return -1;
	}
	ServerLog(SERVERLOG_STATUS, "Initialized scripting engine.");

	result = AngelScriptManager_InitAPI(&server->as_manager);
	if (FAILURE(result))
	{
		ServerLog(SERVERLOG_ERROR, "Failed to initialize script API.\n");
		return -1;
	}
	ServerLog(SERVERLOG_STATUS, "Registered script API.");
	return result;
}

static int Server_LoadConfiguration(struct Server *server)
{
	int result = AngelScriptManager_LoadServerConfig(&server->as_manager,
							&server->configuration);
	if (FAILURE(result))
	{
		ServerLog(SERVERLOG_ERROR,
			"Failed to load configuration file server.cfg.");
		return -1;
	}
	ServerLog(SERVERLOG_STATUS, "Configuration file loaded.");
	return 0;
}

static int Server_LoadGame(struct Server *server)
{
	int result = AngelScriptManager_LoadScripts(&server->as_manager,
						server->configuration.scriptpath);
	if (FAILURE(result))
	{
		ServerLog(SERVERLOG_ERROR, "Failed to load game scripts.");
		return -1;
	}
	ServerLog(SERVERLOG_STATUS, "Game scripts loaded.");

	result = AngelScriptManager_PrepareScriptPersistenceLayer(
		&server->as_manager);
	if (FAILURE(result))
	{
		ServerLog(SERVERLOG_ERROR, "Failed to start persistence layer!");
		return -1;
	}
	else
	{
		ServerLog(SERVERLOG_STATUS, "Persistence layer initialized.");
	}

	if(FAILURE(AngelScriptManager_CallOnServerStartFunc(&server->as_manager)))
	{
		ServerLog(SERVERLOG_ERROR, "OnServerInit() script function returned failure!\n");
		return -1;
	}
	else
	{
		ServerLog(SERVERLOG_STATUS, "OnServerInit() script function return success.\n");
	}

	Server_LoadMOTD(server);

	return 0;
}

static int Server_InitializeThreads(struct Server *server)
{
	MemoryPool_Init(&server->mem_pool, 32);

	server->cpu_cores = get_nprocs();

	if (FAILURE(TickThread_Init(&server->game_tick_thread, server, 1000000)))
	{
		ServerLog(SERVERLOG_ERROR, "Failed to initialize server tick thread!");
		return -1;
	}
	ServerLog(SERVERLOG_STATUS, "Initialized server tick thread.");

	if (FAILURE(
			ThreadPool_Init(&server->thread_pool, max(server->cpu_cores - 2, 1))))
	{
		ServerLog(SERVERLOG_ERROR, "Failed to initialize thread pool!");
		return -1;
	}
	ServerLog(SERVERLOG_STATUS, "Initialized thread pool.");
	return 0;
}

static int Server_InitializeNetwork(struct Server *server, const char *szAddr,
				unsigned short port)
{
	int opts = 1;
	server->sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (FAILURE(server->sockfd))
	{
		ServerLog(SERVERLOG_ERROR, "Could not create socket.");
		return -1;
	}

	if (FAILURE(
			setsockopt(server->sockfd, SOL_SOCKET, SO_REUSEADDR, &opts,
				sizeof(int))))
	{
		ServerLog(SERVERLOG_ERROR, "Failed to set socket options.");
		return -1;
	}

	memset(&(server->addr_in), 0, sizeof(struct sockaddr_in));
	server->addr_in.sin_family = AF_INET;
	server->addr_in.sin_port = htons(port);
	server->addr_in.sin_addr.s_addr = INADDR_ANY;

	// Configure epoll for the server
	server->evlist_len = 64;
	server->evlist = (struct epoll_event*) talloc(
		sizeof(struct epoll_event) * server->evlist_len);
	memset(server->evlist, 0, sizeof(struct epoll_event) * server->evlist_len);

	server->epfd = epoll_create(10);
	server->epkg.sockfd = server->sockfd;
	server->epkg.pData = 0;

	server->server_event.events = EPOLLIN | EPOLLEXCLUSIVE;
	server->server_event.data.ptr = &(server->epkg);

	server->cmdpipe_event.events = EPOLLIN | EPOLLEXCLUSIVE;
	server->cmdpipe_event.data.ptr = server->cmd_pipe;
	//Open a pipe for in-process server commands and have epoll watch the
	//read end

	if (FAILURE(pipe(server->cmd_pipe)) ||
		FAILURE(epoll_ctl(server->epfd, EPOLL_CTL_ADD,
					server->cmd_pipe[0], &(server->cmdpipe_event))))
	{
		ServerLog(SERVERLOG_ERROR, "FAILED TO CREATE COMMAND PIPE!");
		return -1;
	}

	if (FAILURE(
			epoll_ctl(server->epfd, EPOLL_CTL_ADD, server->sockfd,
				&(server->server_event))))
	{
		ServerLog(SERVERLOG_ERROR,
			"FATAL: Could not add server socket to epoll wait list!");
		Server_Stop(server);
		return -1;
	}

	if (FAILURE(
			bind(server->sockfd, (struct sockaddr* ) &server->addr_in,
				sizeof(struct sockaddr_in))))
	{
		ServerLog(SERVERLOG_ERROR, "Failed to bind to %s:%d.",
			inet_ntoa(server->addr_in.sin_addr),
			ntohs(server->addr_in.sin_port));
		return -1;
	}

	int backlog = 64; //TODO: Make this configurable
	if (FAILURE(listen(server->sockfd, backlog)))
	{
		ServerLog(SERVERLOG_ERROR, "Could not listen on %s:%d.",
			inet_ntoa(server->addr_in.sin_addr),
			ntohs(server->addr_in.sin_port));
		return -1;
	}

	ServerLog(SERVERLOG_STATUS, "Server now listening on %s:%d.",
		inet_ntoa(server->addr_in.sin_addr),
		ntohs(server->addr_in.sin_port));

	return 0;
}

int Server_Start(struct Server *server)
{
	memset(server, 0, sizeof(struct Server));

	struct timespec rt, mt;

	clock_gettime(CLOCK_REALTIME, &rt);
	clock_gettime(CLOCK_MONOTONIC, &mt);

	server->realtime_offset.tv_sec = rt.tv_sec - mt.tv_sec;
	server->realtime_offset.tv_nsec = rt.tv_nsec - mt.tv_nsec;

	//rt_offset + mt = current rt

	if (FAILURE(Server_InitializeADTs(server)))
		return -1;
	if (FAILURE(Server_InitializeScriptEngine(server)))
		return -1;

	if (FAILURE(Server_LoadConfiguration(server)))
		return -1;

	if (FAILURE(
			Database_Init(&server->db, server,
				server->configuration.dbfilepath)))
	{
		ServerLog(SERVERLOG_ERROR, "FATAL: Failed to initialize database!");
		Server_Stop(server);
		return -1;
	}
	ServerLog(SERVERLOG_STATUS, "Initialized database engine.");

	if (FAILURE(Server_LoadGame(server)))
		return -1;
	if (FAILURE(Server_InitializeThreads(server)))
		return -1;
	if (FAILURE(
			Server_InitializeNetwork(server, server->configuration.bindip,
						server->configuration.bindport)))
		return -1;
	ServerLog(SERVERLOG_STATUS, "Server running. %d processor cores detected.",
		server->cpu_cores);
	return 0;
}

static void Server_StopNetwork(struct Server *server)
{
	close(server->sockfd);
	close(server->cmd_pipe[0]);
	close(server->cmd_pipe[1]);
	tfree(server->evlist);
}

static void Server_StopThreads(struct Server *server)
{
	ThreadPool_Destroy(&server->thread_pool);
	TickThread_Stop(&server->game_tick_thread);
	MemoryPool_Destroy(&server->mem_pool);
}

static void Server_FreeMOTD(struct Server *server)
{
	tfree(server->MOTD);
	server->MOTD = 0;
}

int Server_Reload(struct Server* server)
{
	/*
	   Clients all possess a reference to a script player_obj which it uses to send player input to
	   the scripts.

	   as_manager.h possesses several references to script functions

	   player.h possesses two references to script functions

	   Every single in-flight RunScriptCmdPkg has a reference to a bunch of
	   script shit. ASAPI_QueueScriptAction and
	   NativeActor::QueueScriptAction create one for an action, and
	   RunScriptAction is used to unpack and execute it.

	 */
	//Server_Stop(server);
	//Server_Teardown(server);
	//Server_Start(server);

	Server_StopThreads(server);

	pthread_rwlock_rdlock(&server->clients_rwlock);
	size_t idx = 0, z = Vector_Count(&server->clients);
	for(; idx < z; ++idx)
	{
		struct Client* client = Vector_At(&server->clients, idx);
		//Release the script objects associated with each client but retain the client
		PlayerConnection_Release(&client->player_obj);
		client->player_obj = 0;
	}
	pthread_rwlock_unlock(&server->clients_rwlock);

	ServerLog(SERVERLOG_STATUS, "Threads should be stopped.");
	Server_UnloadGame(server);
	Server_FreeConfiguration(server);
	Server_StopScriptEngine(server);

	int result = 0;
	result = Server_InitializeScriptEngine(server);
	if(FAILURE(result))
	{
		ServerLog(SERVERLOG_ERROR, "Failed to re-initialize engine!");
		return -1;
	}

	result = Server_LoadConfiguration(server);
	if(FAILURE(result))
	{
		ServerLog(SERVERLOG_ERROR, "Failed to reload configuration!");
		return -1;
	}

	result = Database_Init(&server->db, server,
			server->configuration.dbfilepath);
	if (FAILURE(result))
	{
		ServerLog(SERVERLOG_ERROR, "Failed to reinitialize database!");
		return -1;
	}

	result = Server_LoadGame(server);
	if(FAILURE(result))
	{
		ServerLog(SERVERLOG_ERROR, "Failed to reload game!");

		Server_UnloadGame(server);
		Database_Release(&server->db);
		Server_FreeConfiguration(server);
		Server_StopScriptEngine(server);
		return -1;
	}

	result = Server_InitializeThreads(server);
	if(FAILURE(result))
	{
		ServerLog(SERVERLOG_ERROR, "Failed to restart game threads!");
		return -1;
	}

	pthread_rwlock_rdlock(&server->clients_rwlock);
	for(idx = 0, z = Vector_Count(&server->clients); idx < z; ++idx)
	{
		struct Client* client = Vector_At(&server->clients, idx);
		AngelScriptManager_CallOnPlayerConnect(&server->as_manager, client);
	}
	pthread_rwlock_unlock(&server->clients_rwlock);
	return 0;
}


static void Server_FreeConfiguration(struct Server* server)
{

}

static void Server_UnloadGame(struct Server *server)
{
	Server_FreeMOTD(server);
	Database_Release(&server->db);
}

static void Server_StopScriptEngine(struct Server *server)
{
	AngelScriptManager_ReleaseEngine(&server->as_manager);
	CCompatibleASThreadCleanup();
}

static void Server_ReleaseADTs(struct Server *server)
{
	CryptoManager_Destroy(&server->crypto_manager);
	RandGenerator_Destroy(&server->rand_generator);
	Vector_Destroy(&server->clients);
	pthread_rwlock_destroy(&server->clients_rwlock);
	prioq_destroy(&server->timed_queue);
	pthread_mutex_destroy(&server->timed_queue_mtx);
}

void Server_Stop(struct Server *server)
{
	Server_StopNetwork(server);
	Server_StopThreads(server);
	Server_UnloadGame(server);
	Server_FreeConfiguration(server);
	Server_ReleaseADTs(server);

	//Generally, resources must be released in LIFO order (reverse
	//of how they were acquired).  However, the scripting engine
	//cannot release certain objects while the rest of the mud
	//engine retains references to them.
	Server_StopScriptEngine(server);
}

int Server_Teardown(struct Server* server)
{
	MultiPrecisionLibrary_Teardown();
	return 0;
}
