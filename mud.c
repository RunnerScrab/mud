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


#define SUCCESS(x) (x >= 0)
#define FAILURE(x) (x < 0)

const char *g_ServerLogTypes[] = {"DEBUG", "STATUS", "ERROR"};
const int SERVERLOG_DEBUG = 0;
const int SERVERLOG_STATUS = 1;
const int SERVERLOG_ERROR = 2;

void ServerLog(unsigned int code, const char* fmt, ...)
{
	va_list arglist;
	va_start(arglist, fmt);
	printf("%s: ", g_ServerLogTypes[code]);
	vprintf(fmt, arglist);
	printf("\n");
	va_end(arglist);
}

struct Server
{
	int sockfd;
	struct sockaddr_in addr_in;
	struct Vector clients;

	struct ThreadPool thread_pool;
	unsigned int cpu_cores;
};

int CompClientSock(void* key, void* p)
{
	//THe Vector class and this is awful
	return (*((int*) key) - ((struct Client*) p)->ev_pkg.sockfd);
}

void Server_SendAllClients(struct Server* pServer, const char* msg)
{
	int idx = 0, z = Vector_Count(&(pServer->clients));
	struct Client *pClient = 0;
	for(; idx < z; ++idx)
	{
		pClient = (struct Client*) Vector_At(&(pServer->clients), idx);
		send(pClient->ev_pkg.sockfd, msg, strlen(msg), 0);
	}
}

int Server_Configure(struct Server* server, const char* szAddr, unsigned short port)
{
	int opts = 1;
	server->sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if(FAILURE(server->sockfd))
	{
		ServerLog(SERVERLOG_ERROR, "Could not create socket.");
		return -1;
	}

	if (FAILURE(setsockopt(server->sockfd, SOL_SOCKET, SO_REUSEADDR, &opts, sizeof(int))))
	{
		ServerLog(SERVERLOG_ERROR, "Failed to set socket options.");
		return -1;
	}

	memset(&(server->addr_in), 0, sizeof(struct sockaddr_in));
	server->addr_in.sin_family = AF_INET;
	server->addr_in.sin_port = htons(port);
	inet_pton(AF_INET, szAddr, &(server->addr_in.sin_addr));

	return 0;
}

int Server_Initialize(struct Server* server, unsigned int backlog)
{
	server->cpu_cores = get_nprocs();
	if(FAILURE(ThreadPool_Init(&(server->thread_pool), server->cpu_cores)))
	{
		ServerLog(SERVERLOG_ERROR, "FAILED TO INIT THREAD POOL!");
		return -1;
	}

	ServerLog(SERVERLOG_STATUS, "Server starting. %d cores detected.", server->cpu_cores);

	int result = bind(server->sockfd, (struct sockaddr*) &(server->addr_in),
			sizeof(struct sockaddr_in));

	if(FAILURE(result))
	{
		ServerLog(SERVERLOG_ERROR, "Could not bind to %s:%d.",
			inet_ntoa(server->addr_in.sin_addr), ntohs(server->addr_in.sin_port));
		return -1;
	}

	result = listen(server->sockfd, backlog);

	if(FAILURE(result))
	{
		ServerLog(SERVERLOG_ERROR, "Could not listen on %s:%d.",
			inet_ntoa(server->addr_in.sin_addr), ntohs(server->addr_in.sin_port));
		return -1;
	}

	ServerLog(SERVERLOG_STATUS, "Server listening on %s:%d.",
		inet_ntoa(server->addr_in.sin_addr), ntohs(server->addr_in.sin_port));
	return 0;
}

int Server_Teardown(struct Server* pServer)
{
//	ThreadPool_Stop(&(pServer->thread_pool));
	ThreadPool_Destroy(&(pServer->thread_pool));
	Vector_Destroy(&(pServer->clients));
	close(pServer->sockfd);
	return 0;
}

void* TestHandleClientInput(void* arg)
{
	printf("Received: %s\n", (char*) arg);
	return 0;
}

int main(int argc, char** argv)
{
	InitTallocSystem();
	struct EvPkg
	{
		int sockfd;
		void* pData;
	};

	struct Server server;
	struct EvPkg server_epkg;
	struct epoll_event server_event, clev, evlist[64];
	struct EvPkg* pEvPkg = 0;
	struct Client* pConnectingClient = 0;
	unsigned int addrlen = 0;
	int accepted_sock = 0;
	int epfd = epoll_create(10);
	int ready = 0;
	int loop_ctr = 0;
	ssize_t bytes_read = 0;

	if(FAILURE(Server_Configure(&server, "127.0.0.1", 9001))
		|| FAILURE(Server_Initialize(&server, 32)))
	{
		Server_Teardown(&server);
		return -1;
	}

	server_epkg.sockfd = server.sockfd;
	server_epkg.pData = 0;

	server_event.events = EPOLLIN;
	server_event.data.ptr = &server_epkg;

	if(FAILURE(epoll_ctl(epfd, EPOLL_CTL_ADD, server.sockfd, &server_event)))
	{
		ServerLog(SERVERLOG_ERROR, "FATAL: Could not add server socket to epoll wait list!");
		Server_Teardown(&server);
		return -1;
	}

	memset(evlist, 0, sizeof(struct epoll_event) * 64);

	if(FAILURE(Vector_Create(&(server.clients), 64, Client_Destroy)))
	{
		ServerLog(SERVERLOG_ERROR, "FATAL: Failed to allocate memory for client list!");
		Server_Teardown(&server);
		return -1;
	}

	for(;;)
	{

		ready = epoll_wait(epfd, evlist, 64, -1);

		if(ready == -1)
		{
			continue;
		}

		for(loop_ctr = 0; loop_ctr < ready; ++loop_ctr)
		{
			pEvPkg = (struct EvPkg*) (evlist[loop_ctr].data.ptr);
			if(pEvPkg->sockfd == server.sockfd)
			{
				ServerLog(SERVERLOG_STATUS, "Client connected.\n");
				pConnectingClient = Client_Create();

				accepted_sock = accept(server.sockfd,
						&(pConnectingClient->addr), &addrlen);

				pConnectingClient->ev_pkg.sockfd = accepted_sock;
				pConnectingClient->ev_pkg.pData = pConnectingClient;


				clev.events = EPOLLIN;
				clev.data.ptr = &(pConnectingClient->ev_pkg);

				Vector_Push(&(server.clients), pConnectingClient);

				epoll_ctl(epfd, EPOLL_CTL_ADD, accepted_sock, &clev);
			}
			else
			{
				//TODO: Replace with a real socket reading function (reads as much as needed)
				memset(((struct Client*)pEvPkg->pData)->input_buffer, 0, sizeof(char) * 256);
				bytes_read = read(pEvPkg->sockfd,
						((struct Client*)pEvPkg->pData)->input_buffer,
						256);

				if(bytes_read > 0)
				{
					((struct Client*) pEvPkg->pData)->input_buffer[bytes_read] = 0;
					/* DEMO CODE */
					char* msgcpy = (char*) malloc(sizeof(char) * 256);
					memcpy(msgcpy, ((struct Client*)pEvPkg->pData)->input_buffer, 256 * sizeof(char));
					struct ThreadBundle* tb = ThreadPool_GetLeastBusyThread(&(server.thread_pool));
					if(FAILURE(ThreadPool_AddTask(&(server.thread_pool), tb,
						       TestHandleClientInput, 1, msgcpy, free)))
					{
						ServerLog(SERVERLOG_ERROR, "Failed to add threadpool task!");
					}

					if(strstr(((struct Client*)pEvPkg->pData)->input_buffer, "kill"))
					{
						goto lbl_end_server_loop;
					}
					/* END DEMO CODE */
				}
				else
				{
					int sock = pEvPkg->sockfd;
					ServerLog(SERVERLOG_STATUS, "Client disconnected.");
					size_t foundkey = 0;
					if(FAILURE(Vector_Find(&(server.clients), &sock, CompClientSock, &foundkey)))
					{
						ServerLog(SERVERLOG_DEBUG, "DEBUG: Couldn't find client in vector!");
					}
					else
					{
						//TODO: Get rid of this stupid fucking vector class
						Vector_Remove(&(server.clients), foundkey);
						close(sock);
						epoll_ctl(epfd, EPOLL_CTL_DEL, sock, 0); //This should actually be done automatically, according to man epoll
					}

				}

			}

		}


	}

lbl_end_server_loop:
	ServerLog(SERVERLOG_STATUS, "Server shutting down.");
	Server_SendAllClients(&server, "Server going down!\r\n");

	Server_Teardown(&server);
	printf("%d unfreed allocations.\n", toutstanding_allocs());
	StopTallocSystem();
	return 0;
}
