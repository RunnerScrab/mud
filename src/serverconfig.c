#include "serverconfig.h"
#include <stdlib.h>
#include <string.h>

int ServerConfig_Load(struct ServerConfig* config)
{
	//TODO: This
	memset(config->dbpath, 0, sizeof(char) * 256);
	memset(config->scriptpath, 0, sizeof(char) * 256);
	memset(config->bindip, 0, sizeof(char) * 256);
	config->bindport = 0;
	return 0;
}

void ServerConfig_Destroy(struct ServerConfig* config)
{

}
