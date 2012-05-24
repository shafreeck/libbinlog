#ifndef __DATASOURCE__H
#define __DATASOURCE__H
#include "mysql.h"
#include "constant.h"
#include <stdio.h>
typedef struct datasource_st{
	/*1 for success and 0 for error*/
	int (*connect)(struct datasource_st *ds);
	int (*close)(struct datasource_st *ds);
	unsigned char *(*getEvent)(struct datasource_st *ds);
	void (*freeEvent)(struct datasource_st *ds);

	/*Common fields of net and file*/
	char *url;
	char logfile[256];
	uint32_t position;
	uint32_t index;
	int serverid;

	union {
		struct{
			const char *host;
			int port;
			const char *user;
			const char *passwd;
			MySQL mysql;
			char buf[BL_EVENT_BUF_SIZE];
			char *newbuf;
		}net;
		struct{
			FILE *fp;
			const char *path;
			char buf[BL_EVENT_BUF_SIZE];
			char *newbuf;
		}file;
	}driver;
	char errstr[BL_ERROR_SIZE];
}DataSource;
void dsInitWithMySQL(DataSource *ds,const char *url);
void dsInitWithFile(DataSource *ds,const char *url);
#define dsConnect(ds) ((ds)->connect((ds)))
#define dsClose(ds) ((ds)->close((ds)))
#define dsGetEvent(ds) ((ds)->getEvent((ds)))
#define dsFreeEvent(ds) ((ds)->freeEvent((ds)))

#endif
