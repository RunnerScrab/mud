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

struct TelOpts
{

};

struct Client
{
     int sockfd;
     struct sockaddr addr;
     int tel_stream_state;
     struct TelOpts tel_opts;
     unsigned char tel_cmd_buffer[64];
     unsigned char* input_buffer;
};

struct Server
{
     int sockfd;
     struct sockaddr_in addr_in;
};

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
     close(pServer->sockfd);
     return 0;
}


int main(int argc, char** argv)
{
     struct Server server;
     if(FAILURE(Server_Configure(&server, "127.0.0.1", 9001))
	|| FAILURE(Server_Initialize(&server, 32)))
     {
	  Server_Teardown(&server);
	  return -1;
     }

     struct sockaddr connecting_addr;
     unsigned int addrlen = 0;
     int accepted_sock = 0;
     int epfd = epoll_create(10);
     int ready = 0;
     int j = 0;
     struct epoll_event ev;
     ev.events = EPOLLIN;
     ev.data.fd = server.sockfd;


     epoll_ctl(epfd, EPOLL_CTL_ADD, server.sockfd, &ev);
     struct epoll_event evlist[5];
     for(;;){
	 printf("epoll_wait()");
	 ready = epoll_wait(epfd, evlist, 5, -1);
	 printf("epoll_wait() returns %d\n", ready);
	 if(ready == -1)
	     continue;
	 for(j = 0; j < ready; ++j){
	     if(evlist[j].data.fd == server.sockfd){
		 printf("Client connected.\n");
		 accepted_sock = accept(server.sockfd, &connecting_addr, &addrlen);
		 struct epoll_event clev;
		 clev.events = EPOLLIN;
		 clev.data.fd = accepted_sock;
		 epoll_ctl(epfd, EPOLL_CTL_ADD, accepted_sock, &clev);
		 }
	     else
		 {
		     char buf[256] = {0};
		     read(evlist[j].data.fd, buf, 256);
		     printf("Received: %s\n", buf);

		 }

	     }


     }
     printf("Accepted connection.\n");
     static char* msg = "Hello\r\n";
     send(accepted_sock, msg, strlen(msg), 0);

     Server_Teardown(&server);
     printf("%d unfreed allocations.\n", toutstanding_allocs());
     return 0;
}


