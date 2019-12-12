#ifndef CLIENT_H_
#define CLIENT_H_
#include <sys/socket.h>

extern const int TEL_STREAM_STATE_USERINPUT;
extern const int TEL_STREAM_STATE_IAC;
extern const int TEL_STREAM_STATE_SB;
extern const int TEL_STREAM_STATE_SE;
extern const int TEL_STREAM_STATE_CMD3; //Verbs

struct TelOpts
{

};

struct Client
{
	struct EvPkg
	{
		int sockfd;
		void* pData;
	} ev_pkg;

     struct sockaddr addr;
     int tel_stream_state;
     struct TelOpts tel_opts;
     unsigned char tel_cmd_buffer[64];
     char* input_buffer;
};




struct Client* Client_Create();

void Client_Destroy(void* p);


#endif
