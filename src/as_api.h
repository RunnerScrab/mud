#ifndef AS_API_H_
#define AS_API_H_

//This header file can only be included by C++ code
extern "C"
{
#include "server.h"
}
#include "player.h"

#include "as_addons/scripthandle.h"
#include "as_addons/scriptarray.h"

#include <string>


void ASAPI_DebugVariables(struct Server* server, Player* playerobj);
void ASAPI_DebugObject(CScriptHandle obj);
void ASAPI_DebugArray(CScriptArray& arr);

void ASAPI_SendToAll(struct Server* server, std::string& message);
void ASAPI_QueueScriptCommand(struct Server* server, asIScriptObject* obj, unsigned int delay);
void ASAPI_QueueClientScriptCommand(struct Client* pClient, asIScriptObject* obj, unsigned int delay_s, unsigned int delay_ns);
void ASAPI_Log(std::string& message);
void ASAPI_TrimString(const std::string& in, std::string& out);
void ASAPI_HashPassword(const std::string& password, std::string& out);

void ASAPI_SetGameScriptPath(struct ServerConfig* config, std::string& path);
void ASAPI_SetDatabasePathAndFile(struct ServerConfig* config, std::string& path, std::string& filename);
void ASAPI_SetGameBindAddress(struct ServerConfig* config, std::string& addr, unsigned short port);
#endif
