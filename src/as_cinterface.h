#ifndef AS_CINTERFACE_H_
#define AS_CINTERFACE_H_

typedef struct asIScriptObject asIScriptObject;

void CCompatibleASThreadCleanup();
void asIScriptObject_Release(asIScriptObject** p);

#endif
