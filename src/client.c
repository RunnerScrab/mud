#include "client.h"
#include "talloc.h"
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>


struct Client* Client_Create(int sock)
{
	struct Client* pClient = talloc(sizeof(struct Client));

	memset(&pClient->tel_stream, 0, sizeof(TelnetStream));

	ZCompressor_Init(&pClient->zstreams);
	cv_init(&pClient->tel_stream.sb_args, 32);

	pClient->tel_stream.sock = sock;
	pClient->sock = sock;
	pClient->ev_pkg.sockfd = sock;
	pClient->ev_pkg.pData = pClient;

	clock_gettime(CLOCK_BOOTTIME, &(pClient->connection_time));
	clock_gettime(CLOCK_BOOTTIME, &(pClient->last_input_time));
	pClient->interval_idx = 0;

	cv_init(&pClient->input_buffer, CLIENT_MAXINPUTLEN);
	memset(pClient->cmd_intervals, 0, sizeof(float) * 6);

	return pClient;
}

void Client_Destroy(void* p)
{
	struct Client* pClient = (struct Client*) p;

	cv_destroy(&pClient->tel_stream.sb_args);
	cv_destroy(&(pClient->input_buffer));
	ZCompressor_StopAndRelease(&pClient->zstreams);
	tfree(pClient);
}


void Client_SendMsg(struct Client* pTarget, const char* fmt, ...)
{
	va_list arglist;
	va_start(arglist, fmt);
	vdprintf(pTarget->sock, fmt, arglist);
	va_end(arglist);
}
