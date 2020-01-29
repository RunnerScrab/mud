#include "angelscript.h"

extern "C"
{
	void ASThreadCleanup()
	{
		asThreadCleanup();
	}

	void asIScriptObject_Release(asIScriptObject** p)
	{
		if(p && *p)
		{
			(*p)->Release();
			*p = 0;
		}
	}
}
