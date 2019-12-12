#include "client.h"
#include "talloc.h"

#include <string.h>

const int TEL_STREAM_STATE_USERINPUT = 0;
const int TEL_STREAM_STATE_IAC = 1;
const int TEL_STREAM_STATE_SB = 2;
const int TEL_STREAM_STATE_SE = 3;
const int TEL_STREAM_STATE_CMD3 = 4; //Verbs

struct Client* Client_Create()
{
	struct Client* pClient = (struct Client*) talloc(sizeof(struct Client), __FUNCTION__);
	pClient->tel_stream_state = TEL_STREAM_STATE_USERINPUT;
	memset(&(pClient->tel_cmd_buffer), 0, sizeof(char) * 64);
	pClient->input_buffer = (char*) talloc(sizeof(char) * 256, __FUNCTION__);
	return pClient;
}

void Client_Destroy(void* p)
{
	struct Client* pClient = (struct Client*) p;
	if(pClient->input_buffer)
	{
		tfree(pClient->input_buffer);
		pClient->input_buffer = 0;
	}
	tfree(pClient);
}

