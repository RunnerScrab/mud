#ifndef AS_API_H_
#define AS_API_H_
#include "server.h"
#include <string>

void ASAPI_SendToAll(struct Server* server, std::string& message);
void ASAPI_QueueScriptCommand(struct Server* server, asIScriptObject* obj, unsigned int delay);
void ASAPI_Log(std::string& message);
void ASAPI_TrimString(const std::string& in, std::string& out);
void ASAPI_HashPassword(const std::string& password, std::string& out);
void ASAPI_GenerateUUID(std::string& out);
#endif
