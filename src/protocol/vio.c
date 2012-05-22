#include "vio.h"
#include <errno.h>
#include <unistd.h>
#include <string.h>

int vioFileRead(vio *io,char *buf,size_t len){
	FILE *fp = io->io.file.fp;
	return fread(buf,len,1,fp);
}
int vioFileWrite(vio *io,const char *buf,size_t len){
	FILE *fp = io->io.file.fp;
	return fwrite(buf,len,1,fp);
}
off_t vioFileTell(vio *io){
	FILE *fp = io->io.file.fp;
	return ftell(fp);
}
/*we must read len bytes so return 1 on successful or 0 on error*/
int vioNetRead(vio *io,char *buf,size_t len){
	int fd = io->io.net.fd;
	int nread = 0;
	while(len > 0){
		buf += nread;
		nread = read(fd,buf,len);
		if(nread == -1){
			if(errno == EAGAIN || errno == EWOULDBLOCK)	continue;
			else return 0;
		}
		len -= nread;
		if(nread == 0 && len >0) return 0;
	}
	return 1;
}
/*we must write len bytes so return 1 on successful or 0 on error*/
int vioNetWrite(vio *io,const char *buf,size_t len){
	int fd = io->io.net.fd;
	int nwrite = 0;
	while(len >0){
		buf += nwrite;
		nwrite = write(fd, buf,len);
		if(nwrite == -1){
			if(errno == EAGAIN||errno == EWOULDBLOCK)continue;
			else return 0;
		}
		len -= nwrite;
		if(nwrite == 0 && len >0) return 0;
	}
	return 1;
}

int vioBufferRead(vio *io,char *buf,size_t len){
	off_t blen = io->io.buffer.len;
	off_t pos = io->io.buffer.pos;
	if(blen-pos < len) return 0;
	memcpy(buf,io->io.buffer.buf+pos,len);
	io->io.buffer.pos += len;
	return 1;
}
int vioBufferWrite(vio *io,const char *buf,size_t len){
	off_t blen = io->io.buffer.len;
	off_t pos = io->io.buffer.pos;
	if(blen - pos < len){
		off_t newlen = 2*(blen + len);
		io->io.buffer.buf = realloc(io->io.buffer.buf,newlen);
		io->io.buffer.len = newlen;
	}
	memcpy(io->io.buffer.buf + pos,buf,len);
	io->io.buffer.pos += len;
	return 1;
}
off_t vioBufferTell(vio *io){
	return io->io.buffer.pos;
}


void vioInitWithFile(vio *v,FILE *fp){
	v->read = vioFileRead;	
	v->write = vioFileWrite;	
	v->tell = vioFileTell;	
	v->io.file.fp = fp;
}
void vioInitWithNet(vio *v,int fd){
	v->read = vioNetRead;
	v->write = vioNetWrite;
	v->tell = NULL;
	v->io.net.fd = fd;
}
void vioInitWithBuffer(vio *v,char *buf,size_t len){
	v->read = vioBufferRead;
	v->write = vioBufferWrite;
	v->tell = vioBufferTell;
	v->io.buffer.buf = buf;
	v->io.buffer.len = len;
	v->io.buffer.pos = 0;
}
