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
#include "constants.h"

#define SUCCESS(x) (x >= 0)
#define FAILURE(x) (x < 0)

int main(int argc, char** argv)
{
#ifdef DEBUG
	ServerLog(SERVERLOG_STATUS, "*****DEBUG BUILD*****");
#endif
	struct Server server;

	//Temporary stuff for connecting client - should be put in struct Client

	struct EvPkg* pEvPkg = 0;

	int ready = 0;
	int loop_ctr = 0;
	volatile char mudloop_running = 1;
	//TODO: Handle binding properly w/ ipv6 support
	if(FAILURE(Server_Configure(&server, "127.0.0.1", SERVER_PORT))
		|| FAILURE(Server_Initialize(&server, SERVER_LISTENQUEUELEN)))
	{
		//Server_Teardown(&server);
		return -1;
	}

	for(;mudloop_running;)
	{

		ready = epoll_wait(server.epfd, server.evlist, server.evlist_len, -1);

		if(ready == -1)
		{
			ServerLog(SERVERLOG_ERROR, "Ready -1\n");
			//break;
		}

		for(loop_ctr = 0; loop_ctr < ready; ++loop_ctr)
		{
			pEvPkg = (server.evlist[loop_ctr].data.ptr);
			if(pEvPkg->sockfd == server.sockfd)
			{
				Server_AcceptClient(&server);
			}
			//HACK: ugh, I should make "EvPkg" both more general and descriptive
			else if(server.evlist[loop_ctr].data.ptr == server.cmd_pipe)
			{
				//Received something on cmd pipe
				char buf[256] = {0};
				size_t bread = read(server.cmd_pipe[0], buf, 256);
				buf[bread - 1] = 0;
				printf("Received on cmd pipe: %s\n", buf);
				if(strstr(buf, "kill"))
				{
					Server_SendAllClients(&server, "\r\nServer going down!\r\n");
					mudloop_running = 0;
					break;
				}
			}
			else
			{
				Server_HandleUserInput(&server, pEvPkg->pData);
			}

		}


	}


	ServerLog(SERVERLOG_STATUS, "Server shutting down.");


	Server_Teardown(&server);
	tprint_summary();
	printf("%d unfreed allocations.\n", toutstanding_allocs());
	talloc_subsys_release();
	return 0;
}
