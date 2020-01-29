#ifndef AS_API_H_
#define AS_API_H_
#include "server.h"
#include "player.h"
#include <string>


void ASAPI_DebugVariables(struct Server* server, Player* playerobj);

void ASAPI_SendToAll(struct Server* server, std::string& message);
void ASAPI_QueueScriptCommand(struct Server* server, asIScriptObject* obj, unsigned int delay);
void ASAPI_Log(std::string& message);
void ASAPI_TrimString(const std::string& in, std::string& out);
void ASAPI_HashPassword(const std::string& password, std::string& out);
void ASAPI_GenerateUUID(std::string& out);

void ASAPI_SetGameScriptPath(struct ServerConfig* config, std::string& path);
void ASAPI_SetDatabasePath(struct ServerConfig* config, std::string& path);
void ASAPI_SetGameBindAddress(struct ServerConfig* config, std::string& addr, unsigned short port);
#endif
