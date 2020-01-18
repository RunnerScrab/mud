#ifndef PLAYER_H_
#define PLAYER_H_
#include "client.h"
#include "as_manager.h"
#include <string>

class asIScriptObject;
class asILockableSharedBool;
class asIScriptEngine;
class asIScriptModule;

class Player
{
public:
  struct Client* m_pClient;
  void AddRef();
  void Release();
  void Send(std::string& str);
  static Player* Factory();
protected:
  Player(asIScriptObject* obj);
  ~Player();

  int m_refCount;
  asILockableSharedBool* m_isDead;
  asIScriptObject* m_obj;

};

int LoadPlayerScript(asIScriptEngine* engine, asIScriptModule* module);
int RegisterPlayerProxyClass(asIScriptEngine* engine, asIScriptModule* module);
asIScriptObject* CreatePlayerProxy(AngelScriptManager* manager, struct Client* pClient);
#endif
