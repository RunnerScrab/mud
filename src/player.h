#ifndef PLAYER_H_
#define PLAYER_H_
#include "client.h"

class asIScriptObject;
class asILockableSharedBool;
class asIScriptEngine;
class asIScriptModule;

class Player
{
public:
  struct Client* pClient;
  void AddRef();
  void Release();
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
#endif
