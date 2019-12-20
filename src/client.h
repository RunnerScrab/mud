#ifndef CLIENT_H_
#define CLIENT_H_
#include <sys/socket.h>
#include <stdarg.h>
#include <time.h>
#include "telnet.h"
#include "constants.h"
#include "charvector.h"
#include "zcompressor.h"

struct EvPkg
{
	int sockfd;
	void* pData;
};

struct Client
{
	int sock;
	struct EvPkg ev_pkg;
	struct sockaddr addr;

	TelnetStream tel_stream;
	ZCompressor zstreams;

	struct timespec connection_time;
	struct timespec last_input_time;
	float cmd_intervals[CLIENT_STOREDCMDINTERVALS]; // average commands per second sent
	unsigned char interval_idx;

	cv_t input_buffer;

};

void Client_SendMsg(struct Client* pTarget, const char* fmt, ...);
struct Client* Client_Create(int sock);
void Client_Destroy(void* p);

#endif
