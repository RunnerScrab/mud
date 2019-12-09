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

	unsigned int cpu_cores;
};

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
	int result = 0, opts = 1;
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
	Vector_Destroy(&(pServer->clients));
	close(pServer->sockfd);
	return 0;
}


int main(int argc, char** argv)
{
	struct EvPkg
	{
		int sockfd;
		void* pData;
	};

	struct Server server;
	struct sockaddr connecting_addr;
	struct EvPkg server_epkg;
	struct epoll_event server_event, clev, evlist[64];
	struct EvPkg* pEvPkg = 0;
	struct Client* pConnectingClient = 0;
	unsigned int addrlen = 0;
	int accepted_sock = 0;
	int epfd = epoll_create(10);
	int ready = 0;
	int loop_ctr = 0;
	int bytes_read = 0;

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
			pEvPkg = (evlist[loop_ctr].data.ptr);
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
				bytes_read = read(pEvPkg->sockfd,
					((struct Client*)pEvPkg->pData)->input_buffer,
					256);
				((struct Client*)pEvPkg->pData)->input_buffer[bytes_read] = 0;

				printf("Received: %s\n",
					((struct Client*)pEvPkg->pData)->input_buffer);
				if(strstr(((struct Client*)pEvPkg->pData)->input_buffer, "kill"))
				{
					goto lbl_end_server_loop;
				}

			}

		}


	}

lbl_end_server_loop:
	ServerLog(SERVERLOG_STATUS, "Server shutting down.");
	Server_SendAllClients(&server, "Server going down!\r\n");

	Server_Teardown(&server);
	printf("%d unfreed allocations.\n", toutstanding_allocs());
	return 0;
}
