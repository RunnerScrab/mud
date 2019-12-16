#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#define CLIENT_STOREDCMDINTERVALS 6 //How many intervals to calculate average cmd/s from, for throttling
#define CLIENT_MAXCMDRATE 10.f //How many cmd/s a user may achieve before being kicked
#define CLIENT_MAXINPUTLEN 512 //Maximum length of user input
#define CLIENT_MAXTELCMDLEN 64 //Maximum length of telnet command

#define SERVER_PORT 9001
#define SERVER_LISTENQUEUELEN 64

#endif
