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
#include "server.h"

#define SUCCESS(x) (x >= 0)
#define FAILURE(x) (x < 0)

void* TestHandleClientInput(void* arg)
{
	printf("Received: %s\n", (char*) arg);
	return ((void*) 0);
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


void Server_HandleUserInput(struct Server* pServer, struct Client* pClient, int clientsock)
{
	//TODO: Replace with a real socket reading function (reads as much as needed)
	memset(pClient->input_buffer, 0, sizeof(char) * 256);
	size_t bytes_read = read(clientsock, pClient->input_buffer, 256);

	if(bytes_read > 0)
	{
		/* DEMO CODE */
		pClient->input_buffer[bytes_read] = 0;

		char* msgcpy = talloc(sizeof(char) * 256);
		memcpy(msgcpy, pClient->input_buffer, 256 * sizeof(char));
		if(FAILURE(ThreadPool_AddTask(&(pServer->thread_pool),
							TestHandleClientInput, 1, msgcpy, tfree2)))
		{
			ServerLog(SERVERLOG_ERROR, "Failed to add threadpool task!");
		}

		if(strstr(pClient->input_buffer, "kill"))
		{
			//TODO: Send a kill signal to the process
			//goto lbl_end_server_loop;
			printf("No actual kill function yet.\n");
		}
		/* END DEMO CODE */
	}
	else
	{
		ServerLog(SERVERLOG_STATUS, "Client disconnected.");
		size_t foundkey = 0;
		if(FAILURE(Vector_Find(&(pServer->clients), &clientsock, CompClientSock, &foundkey)))
		{
			ServerLog(SERVERLOG_DEBUG, "DEBUG: Couldn't find client in vector!");
		}
		else
		{
			Vector_Remove(&(pServer->clients), foundkey);
			close(clientsock);
			//This should actually be done automatically, according to man epoll
			epoll_ctl(pServer->epfd, EPOLL_CTL_DEL, clientsock, 0);
		}

	}

}

int main(int argc, char** argv)
{


	struct Server server;
	struct EvPkg server_epkg;
	struct epoll_event server_event, clev, evlist[64];
	struct EvPkg* pEvPkg = 0;
	struct Client* pConnectingClient = 0;
	unsigned int addrlen = 0;
	int accepted_sock = 0;
	server.epfd = epoll_create(10);
	int ready = 0;
	int loop_ctr = 0;

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

	if(FAILURE(epoll_ctl(server.epfd, EPOLL_CTL_ADD, server.sockfd, &server_event)))
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

		ready = epoll_wait(server.epfd, evlist, 64, -1);

		if(ready == -1)
		{
			printf("Ready -1\n");
			break;
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

				epoll_ctl(server.epfd, EPOLL_CTL_ADD, accepted_sock, &clev);
			}
			else
			{
				Server_HandleUserInput(&server, pEvPkg->pData, pEvPkg->sockfd);
			}

		}


	}

	ServerLog(SERVERLOG_STATUS, "Server shutting down.");
	Server_SendAllClients(&server, "Server going down!\r\n");

	Server_Teardown(&server);
	tprint_summary();
	printf("%d unfreed allocations.\n", toutstanding_allocs());
	return 0;
}
