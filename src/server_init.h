#ifndef SERVER_INIT_H_
#define SERVER_INIT_H_

int Server_InitializeADTs(struct Server* server);
int Server_InitializeScriptEngine(struct Server* server);
int Server_LoadConfiguration(struct Server* server);
int Server_LoadGame(struct Server* server);
int Server_InitializeThreads(struct Server* server);
int Server_InitializeNetwork(struct Server* server, const char* szAddr, unsigned short port);
int Server_Start(struct Server* server);
#endif
