#ifndef SERVERCONFIG_H_
#define SERVERCONFIG_H_

struct ServerConfig
{
	char dbfilepath[256];
	char scriptpath[256];
	char bindip[32];
	unsigned short bindport;
};

#endif
