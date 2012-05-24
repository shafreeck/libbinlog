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

/*copied from include/mysql_com.h*/
enum _mservercmd {

	COM_SLEEP, COM_QUIT, COM_INIT_DB, COM_QUERY, COM_FIELD_LIST,
	COM_CREATE_DB, COM_DROP_DB, COM_REFRESH, COM_SHUTDOWN, COM_STATISTICS,
	COM_PROCESS_INFO, COM_CONNECT, COM_PROCESS_KILL, COM_DEBUG, COM_PING,
	COM_TIME, COM_DELAYED_INSERT, COM_CHANGE_USER, COM_BINLOG_DUMP,
	COM_TABLE_DUMP, COM_CONNECT_OUT, COM_REGISTER_SLAVE,
	COM_STMT_PREPARE, COM_STMT_EXECUTE, COM_STMT_SEND_LONG_DATA, COM_STMT_CLOSE,
	COM_STMT_RESET, COM_SET_OPTION, COM_STMT_FETCH, COM_DAEMON,
	/* don't forget to update const char *command_name[] in sql_parse.cc */

	/* Must be last */
	COM_END
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

#endif
