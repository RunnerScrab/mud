#include "angelscript.h"

extern "C"
{
	void ASThreadCleanup()
	{
		asThreadCleanup();
	}

	void asIScriptObject_Release(void** p)
	{
		if(p && *p)
		{
			asIScriptObject* pObj = *reinterpret_cast<asIScriptObject**>(p);
			pObj->Release();
			*p = 0;
		}
	}
}
