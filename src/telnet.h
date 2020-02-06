#ifndef TELNET_H_
#define TELNET_H_
#include "charvector.h"
#include <stdlib.h>
#include <stdio.h>
#include "constants.h"

#define TELSTATE_INPUT 0 //No commands
#define TELSTATE_IAC 1 //IAC received
#define TELSTATE_SB 2 //Suboption negotiation received
#define TELSTATE_SE 3 //End of Suboption negotiation (following an IAC)
#define TELSTATE_CMD 4 //3 byte command likely used in major option negotiation
#define TELSTATE_ERROR 5 //Stream has entered invalid state and does not conform to any known telnet RFC

typedef struct
{
	//To support, 857, 858, 859, 1091, 1073, 1079
	unsigned char b_transmit_binary : 1;
	unsigned char b_echo : 1;
	unsigned char b_sga : 1;
	unsigned char b_naws_set : 1;
	unsigned char b_term_speed_set : 1;
	unsigned char b_mccp2 : 1;
	unsigned char b_mccp3 : 1;
	unsigned char b_termtype_set : 1;

	char terminal_type[42]; //Maximum terminal type name is 40 characters by RFC 930
	unsigned short windowsize_rows, windowsize_cols;
	unsigned short terminal_speed;

} TelnetOptions;

typedef struct
{
	int sock;
	cv_t sb_args;
	unsigned char state;
	TelnetOptions opts;
	unsigned char last_byte;

} TelnetStream;

extern const char *telcodenames[256];

int TelnetStream_SendPreamble(TelnetStream* stream);
int TelnetStream_ProcessByte(TelnetStream* stream, unsigned char x, cv_t* normal_char_dump);

#endif
