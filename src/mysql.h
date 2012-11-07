#ifndef M_MYSQL_H
#define M_MYSQL_H
#include <stdint.h>
#include <stdlib.h>
typedef struct _mysql{
	int fd; /*Or use vio instead ?*/
	uint32_t scapacity;/*server capacity*/
	uint32_t ccapacity;/*client capacity*/
	uint32_t threadid;
	uint32_t maxpkt;
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
#define M_MAX_PACKET_SIZE 1024*1024*16

/*copied from include/mysql_com.h*/
enum _mservercmd {

	M_COM_SLEEP, M_COM_QUIT, M_COM_INIT_DB, M_COM_QUERY, M_COM_FIELD_LIST,
	M_COM_CREATE_DB, M_COM_DROP_DB, M_COM_REFRESH, M_COM_SHUTDOWN, M_COM_STATISTICS,
	M_COM_PROCESS_INFO, M_COM_CONNECT, M_COM_PROCESS_KILL, M_COM_DEBUG, M_COM_PING,
	M_COM_TIME, M_COM_DELAYED_INSERT, M_COM_CHANGE_USER, M_COM_BINLOG_DUMP,
	M_COM_TABLE_DUMP, M_COM_CONNECT_OUT, M_COM_REGISTER_SLAVE,
	M_COM_STMT_PREPARE, M_COM_STMT_EXECUTE, M_COM_STMT_SEND_LONG_DATA, M_COM_STMT_CLOSE,
	M_COM_STMT_RESET, M_COM_SET_OPTION, M_COM_STMT_FETCH, M_COM_DAEMON,
	/* don't forget to update const char *command_name[] in sql_parse.cc */

	/* Must be last */
	M_COM_END
};
/*We add prefix M to distinct from orginal mysql definition*/
#define M_CLIENT_LONG_PASSWORD    1   /* new more secure passwords */
#define M_CLIENT_FOUND_ROWS   2   /* Found instead of affected rows */
#define M_CLIENT_LONG_FLAG    4   /* Get all column flags */
#define M_CLIENT_CONNECT_WITH_DB  8   /* One can specify db on connect */
#define M_CLIENT_NO_SCHEMA    16  /* Don't allow database.table.column */
#define M_CLIENT_COMPRESS     32  /* Can use compression protocol */
#define M_CLIENT_ODBC     64  /* Odbc client */
#define M_CLIENT_LOCAL_FILES  128 /* Can use LOAD DATA LOCAL */
#define M_CLIENT_IGNORE_SPACE 256 /* Ignore spaces before '(' */
#define M_CLIENT_PROTOCOL_41  512 /* New 4.1 protocol */
#define M_CLIENT_INTERACTIVE  1024    /* This is an interactive client */
#define M_CLIENT_SSL              2048    /* Switch to SSL after handshake */
#define M_CLIENT_IGNORE_SIGPIPE   4096    /* IGNORE sigpipes */
#define M_CLIENT_TRANSACTIONS 8192    /* Client knows about transactions */
#define M_CLIENT_RESERVED         16384   /* Old flag for 4.1 protocol  */
#define M_CLIENT_SECURE_CONNECTION 32768  /* New 4.1 authentication */
#define M_CLIENT_MULTI_STATEMENTS (1UL << 16) /* Enable/disable multi-stmt support */
#define M_CLIENT_MULTI_RESULTS    (1UL << 17) /* Enable/disable multi-results */
#define M_CLIENT_PS_MULTI_RESULTS (1UL << 18) /* Multi-results in PS-protocol */
#define M_CLIENT_PLUGIN_AUTH  (1UL << 19) /* Client supports plugin authentication */


#endif
