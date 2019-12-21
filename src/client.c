#include "client.h"
#include "talloc.h"
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include "zcompressor.h"
#include "iohelper.h"

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

//This should be used for all client writes, since it knows whether or not
//to perform stream compression
int Client_WriteTo(struct Client* pTarget, const char* buf, size_t len)
{
	switch(pTarget->tel_stream.opts.b_mccp2)
	{
	case 1:
	{
		cv_t compout;
		cv_init(&compout, len);
		if(ZCompressor_CompressRawData(&pTarget->zstreams,
						buf, len, &compout) < 0)
		{
			return -1;
		}

		int result = write_from_cv_raw(pTarget->sock, &compout);
		cv_destroy(&compout);
		return result;
	}
	default:
		return write_full_raw(pTarget->sock, buf, len);
	}
}

void Client_Sendf(struct Client* pTarget, const char* fmt, ...)
{
	va_list arglist, argcpy;
	va_start(arglist, fmt);
	va_copy(argcpy, arglist);
	cv_t buf;
	cv_init(&buf, 512);
	size_t required = vsnprintf(buf.data, 512, fmt, arglist);
	cv_resize(&buf, required);

	required = vsnprintf(buf.data, required, fmt, argcpy);
	buf.length = required;
	Client_WriteTo(pTarget, buf.data, buf.length);

	cv_destroy(&buf);
	va_end(arglist);
	va_end(argcpy);
}
