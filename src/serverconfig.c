#include "serverconfig.h"
#include <stdlib.h>
#include <string.h>

int ServerConfig_Load(struct ServerConfig *config)
{
	memset(config, 0, sizeof(struct ServerConfig));
	return 0;
}

void ServerConfig_Destroy(struct ServerConfig *config)
{

}
