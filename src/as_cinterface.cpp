#include "angelscript.h"
#include "player.h"

extern "C"
{
	void CCompatibleASThreadCleanup()
	{
		asThreadCleanup();
	}

	void PlayerConnection_Release(PlayerConnection** conn)
	{
		if(conn && *conn)
		{
			(*conn)->Release();
			*conn = 0;
		}
	}

	void asIScriptObject_Release(asIScriptObject **p)
	{
		if (p && *p)
		{
			(*p)->Release();
			*p = 0;
		}
	}
}
