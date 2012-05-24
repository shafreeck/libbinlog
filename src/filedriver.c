#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "datasource.h"


static unsigned char *getEventFromFile(DataSource *ds){
	int c = 0;
	FILE *fp = ds->driver.file.fp;
	unsigned char header[19]={0};
	uint64_t position = ds->position;
	if(position <4){
		position = 4;
	}
	unsigned char *p = header;
	c = fread(header,19,1,fp);
	if(c==-1){
		snprintf(ds->errstr,BL_ERROR_SIZE,"getEventFromFile:%s",strerror(errno));
		return NULL;
	}
	if(c==0){
		return NULL;
	}
	int len = *(int*)(header+9);
	unsigned char *ev =(unsigned  char*) ds->driver.file.buf;
	ds->driver.file.newbuf=NULL;
	if(len>BL_EVENT_BUF_SIZE){
		ev = malloc(len);
		ds->driver.file.newbuf=(char*)ev;
	}
	/*Copy header*/
	memcpy(ev,header,19);
	len -= 19;

	p = ev+19;
	c = fread(p,len,1,fp);
	if(c==-1){
		snprintf(ds->errstr,BL_ERROR_SIZE,"getEventFromFile:%s",strerror(errno));
		return NULL;
	}
	return ev;
}

static int readMagic(char *target,int tlen,FILE *fp){
	if(tlen < 4){
		return -1;
	}
	return fread(target,4,1,fp);
}
static int openFile(DataSource *ds){
	ds->driver.file.fp = fopen(ds->driver.file.path,"r");
	// read MAGIC
	char magic[4]={0};
	if(readMagic(magic,4,ds->driver.file.fp)<0){
		return 0;	
	}
	return 1;
}
static int closeFile(DataSource *ds){
	free(ds->url);
	fclose(ds->driver.file.fp);
	return 1;
}
static void freeEventFromFile(DataSource *ds){
	if(ds->driver.file.newbuf){
		free(ds->driver.file.newbuf);
	}
}
static DataSource dsFile={
	openFile,
	closeFile,
	getEventFromFile,
	freeEventFromFile,
	NULL,{0},
	4, 0,-1,
	{{0}}
};
void parseFileUrl(DataSource *ds,const char *surl){
	if(*surl!='f')return;
	char *url = strdup(surl);
	ds->url = url;
	int s_path,s_file;
	s_path = s_file = 1;
	char next = ':';
	char *logfile;
	while(*++url!='\0'){
		if(*url ==next){
			url += 3;
			if(s_path && *url=='/'){
				ds->driver.file.path= ++url;
				s_path = 0;
				next  = '/';
				continue;
			}
			if(s_file){
				logfile = url;
			}
		}
	}
	if(!ds->driver.file.path){
		ds->driver.file.path = logfile;
	}
	if(logfile && strlen(logfile)<256)
		strcpy(ds->logfile,logfile);
}
void dsInitWithFile(DataSource *ds,const char* url){
	*ds  = dsFile;
	parseFileUrl(ds,url);
}
