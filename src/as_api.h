#ifndef AS_API_H_
#define AS_API_H_
#include "server.h"
#include <string>

void ASAPI_SendToAll(struct Server* server, std::string& message);

#endif