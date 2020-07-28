#ifndef SERVER_INIT_H_
#define SERVER_INIT_H_

struct Server;

int Server_Start(struct Server *server);
int Server_Reload(struct Server* server);
void Server_Stop(struct Server *server);

#endif
