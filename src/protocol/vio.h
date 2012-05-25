/*
 *An IO implements like redis's rio
 **/
#ifndef M_VIO_H
#define M_VIO_H
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>

typedef struct _vio{
	int (*read)(struct _vio *io,char *buf,size_t len);
	int (*write)(struct _vio *io,const char *buf,size_t len);
	off_t (*tell)(struct _vio *io);
	union {
		struct {
			char *buf;
			off_t len;
			off_t pos;
		}buffer;
		struct {
			FILE *fp;
		}file;
		struct {
			int fd;
		}net;
	}io;
}vio;

void vioInitWithFile(vio *v,FILE *fp); 
void vioInitWithNet(vio *v,int fd); 
void vioInitWithBuffer(vio *v,char *buf,size_t len); 

/*public interface of vio*/
#define vioRead(vio,buf,len) ((vio)->read((vio),(buf),(len)))
#define vioWrite(vio,buf,len) ((vio)->write((vio),(buf),(len)))
#define vioTell(vio) ((vio)->tell((vio)))

#endif
