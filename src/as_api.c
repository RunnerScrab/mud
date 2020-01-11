#include "as_api.h"

void ASAPI_SendToAll(struct Server* server, std::string& message)
{
	Server_SendAllClients(server, message.c_str());
}
