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
#define TELSTATE_SE 3 //End of Suboption negotiation (following an IAC)
#define TELSTATE_CMD 4 //3 byte command likely used in major option negotiation
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

void MakeTelCmd(char* response, unsigned char a,
		unsigned char b, unsigned char c)
{
	response[0] = a;
	response[1] = b;
	response[2] = c;
}

void Run3ByteCmd(TelnetStream* stream, unsigned char x)
{
	unsigned char last_byte = stream->last_byte;
	char response[3] = {0};
	switch(x)
	{
	case MCCP2:
		if(last_byte == DO)
		{
			//DO MCCP2
			printf("ENABLING MCCP2\n");
			stream->opts.b_mccp2 = 1;
			MakeTelCmd(response, IAC, WILL, MCCP2);
		}
		break;
	case MCCP3:
		if(last_byte == DO)
		{
			printf("ENABLING MCCP3\n");
			MakeTelCmd(response, IAC, WILL, MCCP3);
			stream->opts.b_mccp3 = 1;
		}
		break;
	case ECHO:
		break;
	case LM:
		break;
	case SGA:
		response[0] = IAC;
		response[1] = WILL;
		response[2] = SGA;
		//We don't send GAs anyway
		printf("Sent %d %d %d\n",
			255 & response[0], 255 & response[1], 255 & response[2]);
		write_full_raw(stream->sock, response, 3);
		break;
	default:
		if(last_byte == DO || last_byte == WILL)
		{
			response[0] = IAC;
			response[1] = last_byte == DO ? WONT : DONT;
			response[2] = x;
			printf("Sent %d %d %d\n",
				255 & response[0], 255 & response[1], 255 & response[2]);
			write_full_raw(stream->sock, response, 3);

		}
		break;
	}
}

void RunSubnegotiationCmd(TelnetStream* stream)
{

}

//Tell a newly connecting client what the server supports
int TelnetStream_SendPreamble(TelnetStream* stream)
{
	static const char preamble[] = {
		IAC, DO, LM,
		IAC, WILL, ECHO,
		IAC, WILL, SGA,
		IAC, WILL, MCCP2,
		IAC, WILL, MCCP3};
		return write_full_raw(stream->sock, preamble, 15);
}

int TelnetStream_ProcessByte(TelnetStream* stream, unsigned char x)
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

	stream->last_byte = x;

	return *curtelstate != TELSTATE_INPUT;
}


const char *telcodenames[256] =
{
	"TB", //0
	"ECHO", //1
	"RECON", //2
	"SGA", //3
	"AMSN", //4
	"STAT", //5
	"TM", //6
	"RCTE", //7
	"OLW", //8
	"OPS", //9
	"NAOCRD", //10
	"NAOHT", //11
	"NAOHTD", //12
	"NAOFD", //13
	"NAVT", //14
	"NAOVTD", //15
	"NAOLD", //16
	"EA", // 17
	"LOGOUT", //18
	"BM", //19
	"DET", //20
	"SUPDUP", //21
	"SUPDUPO", //22
	"SL", //23
	"TT", //24
	"EOR", //25
	"TACACSUI", //26
	"OM", //27
	"TTYLOC", //28
	"T3270", //29
	"X3", //30
	"NAWS", //31
	"TS", //32
	"RFC", //33
	"LM", //34
	"XDL", //35
	"ENV", //36
	"AUTH", //37
	"EO", //38
	"NE", //39
	"TN3270E", //40
	"XAUTH", //41
	"CHARSET", //42
	"RSP", //43
	"CPC", //44
	"TSLE", //45
	"TLS", //46
	"KERMIT", //47
	"SENDURL", //48
	"FORWARDX", //49
	0,0,0,0,0,0,0,0,0,0,
	"TEST", //60
	0,0,0,0,0, 0,0,0,0,0, //70
	0,0,0,0,0, 0,0,0,0,0, //80
	0,0,0,0,0,
	"MCCP2", //86
	"MCCP3", //87
	0,0,
	0,0,0,0,0, 0,0,0,0,0, //90-99
	0,0,0,0,0, 0,0,0,0,0, //100-109
	0,0,0,0,0, 0,0,0,0,0, //110-119
	0,0,0,0,0, 0,0,0,0,0, //120-129
	0,0,0,0,0, 0,0,0, //130-137
	"PRAGMA", //138
	"SSPI", //139
	"PRAGMAHB", //140
	0,0,0,0,0, 0,0,0,0,0,//141-150
	0,0,0,0,0, 0,0,0,0,0,//151-160
	0,0,0,0,0, 0,0,0,0,0,//170
	0,0,0,0,0, 0,0,0,0,0,//180
	0,0,0,0,0, 0,0,0,0,0,//190
	0,0,0,0,0, 0,0,0,0,0,//200
	0,0,0,0,0, 0,0,0,0,0,//210
	0,0,0,0,0, 0,0,0,0,0,//220
	0,0,0,0,0, 0,0,0,0,0,//230
	0,0,0,0,0, 0,0,0,0, //239
	"SE", //240
	"NOP", //241
	"DM", //242
	"BRK", //243
	"IP", //244
	"AO", //245
	"AYT", //246
	"EC", //247
	"EL", //248
	"GA", //249
	"SB", //250p
	"WILL", //251
	"WONT", //252
	"DO", //253
	"DONT", //254
	"IAC" //255
};
