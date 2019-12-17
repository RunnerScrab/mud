#include "telnet.h"
#include "iohelper.h"

#define IAC 255
#define DONT 254
#define DO 253
#define WONT 252
#define WILL 251

#define SB 250
#define GA 249
#define EL 248
#define EC 247
#define AYT 246
#define AO 245
#define IP 244
#define BRK 243
#define DM 242
#define NOP 241
#define SE 240
#define TEST 60

#define TB 0
#define ECHO 1
#define RECON 2
#define SGA 3
#define AMSN 4
#define STAT 5
#define TM 6
#define RCTE 7
#define OLW 8
#define OPS 9
#define NAOCRD 10
#define NAOHT 11
#define NAOHTD 12
#define NAOFD 13
#define NAOHT 11
#define NAOHTD 12
#define NAOFD 13
#define NAVT 14
#define NAOVTD 15
#define NAOLD 16
#define EA 17
#define LOGOUT 18
#define BM 19
#define DET 20
#define SUPDUP 21
#define SUPDUPO 22
#define SL 23
#define TT 24
#define EOR 25
#define TACACSUI 26
#define OM 27
#define TTYLOC 28
#define T3270 29
#define X3 30
#define NAWS 31
#define TS 32
#define RFC 33
#define LM 34
#define XDL 35
#define ENV 36
#define AUTH 37
#define EO 38
#define NE 39
#define TN3270E 40
#define XAUTH 41
#define CHARSET 42
#define RSP 43
#define CPC 44
#define TSLE 45
#define TLS 46
#define KERMIT 47
#define SENDURL 48
#define FORWARDX 49
#define MCCP2 86
#define MCCP3 87
#define PRAGMA 138
#define SSPI 139
#define PRAGMAHB 140


#define TELSTATE_INPUT 0 //No commands
#define TELSTATE_IAC 1 //IAC received
#define TELSTATE_SB 2 //Suboption negotiation received
#define TELSTATE_CMD 3 //3 byte command likely used in major option negotiation
#define TELSTATE_SE 4 //End of Suboption negotiation (following an IAC)
#define TELSTATE_ERROR 5 //Stream has entered invalid state and does not conform to any known telnet RFC

const unsigned char supported_options[] =
{
	ECHO,
	SGA,
	STAT,
	TT,
	NAWS,
	MCCP2,
	MCCP3
};

void SetTelnetState(unsigned char* curtelstate, unsigned char newval)
{
	printf("Switching to telnet state %u\n", newval);
	*curtelstate = newval;
}

unsigned char GetIs2CmdByte(unsigned char x)
{
	return x >= SE && x <= 250;
}

unsigned char Is3ByteCmd(unsigned char x)
{
	return (x >= 0 && x <= 140) || (x == 86 || x == 87);
}

void Run2ByteCmd(TelnetStream* stream, unsigned char x)
{
	//TODO
}

void Run3ByteCmd(TelnetStream* stream, unsigned char x)
{
	unsigned char response[3] = {0};
	switch(x)
	{
	case SGA:
		response[0] = IAC;
		response[1] = WILL;
		response[2] = SGA;
		//We don't send GAs anyway
		break;
	default:
		response[0] = IAC;
		response[1] = WONT;
		response[2] = x;
		break;
	}
	printf("Sent %d %d %d\n",
		255 & response[0], 255 & response[1], 255 & response[2]);
	write_full(stream->sock, response, 3);
}

void RunSubnegotiationCmd(TelnetStream* stream)
{

}

int ProcessByteForTelnetCmds(TelnetStream* stream, unsigned char x)
{
	unsigned char* curtelstate = &stream->state;
	switch(*curtelstate)
	{
	case TELSTATE_INPUT:
		switch(x)
		{
		case IAC:
			SetTelnetState(curtelstate, TELSTATE_IAC);
			break;
		default:
			//Character is not part of a telnet command and is just a
			//normal input character
			break;
		}
		break;
	case TELSTATE_IAC:
		if(SB == x)
		{
			SetTelnetState(curtelstate, TELSTATE_SB);
		}
		else if(!GetIs2CmdByte(x))
		{
			//Must be a 2 or 3 byte command if it is not SB
			SetTelnetState(curtelstate, TELSTATE_CMD);
		}
		else
		{
			//IS a 2 CMD byte
			Run2ByteCmd(stream, x);
			SetTelnetState(curtelstate, TELSTATE_INPUT); //Go back to input mode (?)
		}
		break;
	case TELSTATE_SB:
		if(IAC == x)
		{
			SetTelnetState(curtelstate, TELSTATE_SE); //We're expecting an SE after this IAC
		}
		else
		{
			//TODO: Begin collecting subnegotiation arguments
		}
		break;
	case TELSTATE_SE:
		if(SE == x)
		{
			RunSubnegotiationCmd(stream);
			SetTelnetState(curtelstate, TELSTATE_INPUT); //TODO: Check if we necessarily go back to input mode
		}
		else
		{
			//SE was the only legal character that could have been sent at this point,
			//but it wasn't
			SetTelnetState(curtelstate, TELSTATE_ERROR);
		}
		break;
	case TELSTATE_CMD:
		if(Is3ByteCmd(x))
		{
			Run3ByteCmd(stream, x);
			SetTelnetState(curtelstate, TELSTATE_INPUT);
		}
		else
		{
			//Apparently received incomplete command
			SetTelnetState(curtelstate, TELSTATE_ERROR);
		}
	default:
		break;

	}
	return *curtelstate != TELSTATE_INPUT;
}
