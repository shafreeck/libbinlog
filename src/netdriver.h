#ifndef __NETDRIVER_H
#define __NETDRIVER_H
#include "libbinlog.h"
#include "mysql.h"
typedef struct netdriver_st{
	int index;
	MySQL *mysql;
	char *fileName;
	uint64_t position;
	uint32_t serverID;
	
	char *newbuf;
	char buf[BL_EVENT_BUF_SIZE];
	char errstr[BL_ERROR_SIZE];

	char *host;
	char *port;
	char *user;
	char *passwd;

}NetDriver;

int connectMySQL(MySQL *mysql);
uint8_t *getEventFromServer(void *args);
void freeEventFromServer(void *args);
#endif
