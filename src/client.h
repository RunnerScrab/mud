#ifndef CLIENT_H_
#define CLIENT_H_
#include <sys/socket.h>
#include <stdarg.h>
#include <time.h>
#include "constants.h"
#include "charvector.h"

extern const int TEL_STREAM_STATE_USERINPUT;
extern const int TEL_STREAM_STATE_IAC;
extern const int TEL_STREAM_STATE_SB;
extern const int TEL_STREAM_STATE_SE;
extern const int TEL_STREAM_STATE_CMD3; //Verbs

struct TelOpts
{

};

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
	int tel_stream_state;
	struct TelOpts tel_opts;
	unsigned char tel_cmd_buffer[CLIENT_MAXTELCMDLEN];

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
