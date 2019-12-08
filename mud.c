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
#include "sockarray.h"

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

void PrintIntBytes(unsigned int bytes)
{
     printf("%d, %d, %d, %d\n",
	    255 & (bytes >> 24),
	    255 & (bytes >> 16),
	    255 & (bytes >> 8),
	    255 & (bytes));
	    
}

struct Server
{
     int sockfd;
     struct sockaddr_in addr_in;
};


int Server_Initialize(struct Server* server, const char* szAddr, unsigned short port)
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

     result = bind(server->sockfd, (struct sockaddr*) &(server->addr_in),
	  sizeof(struct sockaddr_in));
     
     if(FAILURE(result))
     {
	  ServerLog(SERVERLOG_ERROR, "Could not bind to %s:%d.", szAddr, port);
	  return -1;
     }

     result = listen(server->sockfd, 64);

     if(FAILURE(result))
     {
	  ServerLog(SERVERLOG_ERROR, "Could not listen on %s:%d.", szAddr, port);
	  return -1;
     }

     ServerLog(SERVERLOG_STATUS, "Server listening on %s:%d.", szAddr, port);
     return 0;
}

int Server_Teardown(struct Server* pServer)
{
     close(pServer->sockfd);
     return 0;
}


int main(int argc, char** argv)
{
     struct Server server;
     if(FAILURE(Server_Initialize(&server, "127.0.0.1", 9001)))
     {
	  Server_Teardown(&server);
	  return -1;
     }

     struct sockaddr connecting_addr;
     unsigned int addrlen = 0;
     int accepted_sock = accept(server.sockfd, &connecting_addr, &addrlen);
     printf("Accepted connection.\n");
     static char* msg = "Hello\r\n";
     send(accepted_sock, msg, strlen(msg), 0);
     Server_Teardown(&server);
     printf("%d unfreed allocations.\n", toutstanding_allocs());
     return 0;
}


