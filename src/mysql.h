#ifndef LIBBINLOG_MYSQL_H
#define LIBBINLOG_MYSQL_H
#include <stdint.h>
#include <stdlib.h>
typedef struct _mysql{
	int fd; /*Or use vio instead ?*/
	uint32_t scapacity;/*server capacity*/
	uint32_t ccapacity;/*client capacity*/
	uint32_t threadid;
	uint16_t status;
	uint8_t charset;
	uint8_t pnum; /*Packet number of a talk*/
	char errstr[256]; /*OK or error message*/
}MySQL;

int mconnect(MySQL *mysql,const char *host,const int port,const char *user,const char *passwd,const char *db);
int mclose(MySQL *mysql);

int msendcmd(MySQL *mysql,unsigned char cmd,const char *args,int argslen);
int msaferead(MySQL *mysql,char *buf,size_t blen,char **newbuf);

/*TODO implement these functions
int minsert(MySQL *mysql,const char *sqlfmt,...);
int mdelete(MySQL *mysql,const char *sqlfmt,...);
int mupdate(MySQL *mysql,const char *sqlfmt,...);
*/

#endif
