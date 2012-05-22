#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include "datasource.h"
#include "anet.h"

/*search match for chars */
static int smatch(char c,char *s){
	while(*s!='\0'){
		if(c==*s)return 1;
		s++;
	}
	return 0;
}
static void parseMySQLUrl(DataSource *ds,const char *surl){
	char *next;
	char *port;
	int done = 0;
	int s_user,s_passwd,s_host,s_port,s_file;
	s_user=s_passwd=s_host=s_port=s_file = 1;

	char *url = strdup(surl);
	ds->url = url;

	port = NULL;
	if(url[0]=='m'){/*mysql url*/
		next = ":";
		while(*++url!='\0'){
			if(smatch(*url,next)){
				if(s_user){
					s_user = 0;
					ds->driver.net.user = url+3;
					done++;
					next=":@";
					continue;
				}
				if(s_passwd && *url == ':'){
					s_passwd=0;
					*url='\0';
					ds->driver.net.passwd = url+1;	
					done++;
					continue;
					next = "@";
				}
				if( s_host && *url == '@'){
					s_passwd = 0;
					s_host = 0;
					*url='\0';
					ds->driver.net.host = url+1;	
					done++;
					next = ":/";
					continue;
				}
				if(s_port && *url == ':'){
					s_port = 0;
					*url='\0';
					port = url+1;	
					done++;
					next = "/";
					continue;
				}
				if(s_file && *url == '/'){
					s_port = 0;
					s_file = 0;
					*url='\0';
					ds->logfile = url+1;	
					done++;
					continue;
				}
				if(*url == '/'){
					*url='\0';
					done++;
					continue;
				}
			}

		}
	}
	if(port)
		ds->driver.net.port = atoi(port);
}
static int connectMySQL(DataSource *ds){
	MySQL *mysql  = &(ds->driver.net.mysql);
	if(!mconnect(mysql,ds->driver.net.host,ds->driver.net.port,ds->driver.net.user,\
				ds->driver.net.passwd,NULL)){
		snprintf(ds->errstr,BL_ERROR_SIZE,"%s",mysql->errstr);	
		return 0;
	}
	char args[256];
	size_t argslen,slen;
	*(uint32_t*)args = ds->position;
	*(uint16_t*)(args+4) = 0;
	*(uint32_t*)(args+6) = ds->serverid;
	slen = strlen(ds->logfile);
	memcpy(args+10,ds->logfile,slen);
	argslen = 10 + slen;
	/*Send the COM_BINLOG_DUMP command*/
	if(!msendcmd(mysql,0x12,args,argslen)){
		snprintf(ds->errstr,BL_ERROR_SIZE,"%s",mysql->errstr);	
		return 0;
	}
	return 1;
}
static int closeMySQL(DataSource *ds){
	free(ds->url);
	mclose(&(ds->driver.net.mysql));
	return 1;
}
static unsigned char *getEventFromServer(DataSource *ds){
	MySQL *mysql = &(ds->driver.net.mysql);

	char *buf = ds->driver.net.buf;
	int nread = msaferead(mysql,buf,BL_EVENT_BUF_SIZE,&(ds->driver.net.newbuf));
	if(nread<0){
		snprintf(ds->errstr,BL_ERROR_SIZE,"%s",mysql->errstr);
		return NULL;
	};
	if(nread == 0){
		sprintf(ds->errstr,"%s","Read all done , maybe your serverID is 0");
		return NULL;
	}
	if(ds->driver.net.newbuf){
		buf = ds->driver.net.newbuf;
	}
	/*Check the EOF packet*/
	if(nread < 8 && buf[0]==254){
		snprintf(ds->errstr,BL_ERROR_SIZE,"%s",mysql->errstr);
		return NULL;
	}
	return (unsigned char*)buf + 1;
}
static void freeEventFromServer(DataSource *ds){
	if(ds->driver.net.newbuf){
		free(ds->driver.net.newbuf);
	}
}
static DataSource dsMySQL = {
	connectMySQL,
	closeMySQL,
	getEventFromServer,
	freeEventFromServer,
	NULL,NULL,
	4, 0,0,
	{{0}},
};
void dsInitWithMySQL(DataSource *ds,const char*url){
	*ds = dsMySQL;
	parseMySQLUrl(ds,url);
}
