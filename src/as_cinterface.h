#ifndef AS_CINTERFACE_H_
#define AS_CINTERFACE_H_

typedef struct asIScriptObject asIScriptObject;
typedef struct PlayerConnection PlayerConnection;

void CCompatibleASThreadCleanup();
void asIScriptObject_Release(asIScriptObject **p);
void PlayerConnection_Release(PlayerConnection** conn);
#endif
