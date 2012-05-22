#include "../protocol.h"
#include "../anet.h"
#include <stdarg.h>
#include <string.h>
#include <assert.h>

void die(const char *fmt,...){
	va_list ap;
	va_start(ap,fmt);
	vfprintf(stderr,fmt,ap);
	va_end(ap);
}

void test_readHandshakePkt(){
	char err[128],buf[256];
	char *host = "10.210.210.146";
	int port = 3306,nread,nwrite;
	int fd = anetTcpConnect(err,host,port);	
	if(fd<0){
		die("Unable connect to %s:%d",host,port);
	}
		
	PktHeader header;
	readPktHeader(&header,fd);
	printf("plen:%d\n",header.plen);
	printf("pnum:%d\n",header.pnum);
	
	HandshakePkt pkt;
	readHandshakePkt(&pkt,header.plen,fd);
	printf("protocol version:%d\n",pkt.protover);
	printf("mysql version:%s\n",pkt.mysqlver);
	printf("threadID:%d\n",pkt.threadid);
	printf("salt:%s\n",pkt.salt);
	printf("salt length:%d\n",pkt.saltlen);
	printf("server capabilities:%d\n",pkt.capacity);
	printf("server status:%d\n",pkt.status);
	printf("server charset:%d\n",pkt.charset);
	printf("plugin name:%s\n",pkt.pluginname);
}

void test_writeAuthPkt(){
	char err[128],buf[256];
	char *host = "10.210.210.146";
	int port = 3306,nread,nwrite;
	int fd = anetTcpConnect(err,host,port);	
	if(fd<0){
		die("Unable connect to %s:%d",host,port);
	}
	PktHeader header;
	readPktHeader(&header,fd);
	printf("plen:%d\n",header.plen);
	printf("pnum:%d\n",header.pnum);
	
	HandshakePkt hpkt;
	readHandshakePkt(&hpkt,header.plen,fd);

	AuthPkt pkt;
	memset(&pkt,0,sizeof(pkt));
#define CLIENT_PROTOCOL_41 512;

	pkt.clientflags = CLIENT_PROTOCOL_41;
	pkt.maxpkt = 1024*1024*16; /* 2^24 */
	pkt.charset = 8;
	pkt.user = "root";
	pkt.saltlen = 0;
	pkt.database = NULL;
	size_t plen = calcAuthPktLen(&pkt);
	assert(writeHeader(plen,1,fd)==PROTO_OK);
	assert(writeAuthPkt(&pkt,plen,fd)==PROTO_OK);

	readPktHeader(&header,fd);
	printf("auth plen:%d\n",header.plen);
	printf("auth pnum:%d\n",header.pnum);

	ReplyPkt resp;
	readReplyPkt(&resp,header.plen,fd);
	if(resp.isok){
		printf("message:%s\n",resp.pkt.ok.message);
		struct _binlogdump{
			char pos[4];
			char flags[2];
			char threadid[4];
			char file[64];
		} bdcmd;
		memset(&bdcmd,0,sizeof(bdcmd));
		uint32_t pos = 4;
		uint32_t threadid = 1;
		uint16_t flag = 0;
		memcpy(bdcmd.pos,&pos,4);
		memcpy(bdcmd.flags,&flag,2);
		memcpy(bdcmd.threadid,&threadid,4);
		char *file = "mysql-bin.000001";
		memcpy(bdcmd.file,file,strlen(file));
		int size = 4 + 2 + 4 + strlen(file);
		CmdPkt cpkt;
		cpkt.cmd = 0x12;
		cpkt.args =(char*) &bdcmd;
		size += 1;
		writeHeader(size,0,fd);
		writeCmdPkt(&cpkt,size,fd);

		readPktHeader(&header,fd);
		printf("auth plen:%d\n",header.plen);
		printf("auth pnum:%d\n",header.pnum);
		readReplyPkt(&resp,header.plen,fd);
		if(resp.isok){
			printf("message:%s\n",resp.pkt.ok.message);
		}else{
			printf("errno:%d\n",resp.pkt.err.errno);
			printf("smarker:%c\n",resp.pkt.err.smarker);
			printf("message:%s\n",resp.pkt.err.message);
		}

	}else{
		printf("errno:%d\n",resp.pkt.err.errno);
		printf("smarker:%c\n",resp.pkt.err.smarker);
		printf("message:%s\n",resp.pkt.err.message);
	}

}

#define ARRLEN(arr) (sizeof(arr)/sizeof((arr)[0]))
int main(int argc, const char *argv[]) {
	test_readHandshakePkt();
	test_writeAuthPkt();
	return 0;
}

#if 0 /*Old test*/
#define resetIO(io)((io).io.buffer.pos=0)

int main(int argc, const char *argv[]) {
	char err[128],buf[256];
	char *host = "10.210.210.146";
	int port = 3306,nread,nwrite;
	int fd = anetTcpConnect(err,host,port);	
	if(fd<0){
		die("Unable connect to %s:%d",host,port);
	}
	nread = anetRead(fd,buf,4);
	vio io;
	vioInitWithBuffer(&io,buf,sizeof(buf));
	//struct chpktHeader *header = (struct chpktHeader*)buf;
	size_t plen,pnum;
	printf("nread:%d\n",nread);
	printf("plen:%d\n",plen=readUint24(&io));
	printf("pnum:%d\n",pnum=readUint8(&io));

	nread = anetRead(fd,buf,plen);
	printf("nread:%d\n",nread);
	resetIO(io);
	printf("protover:%d\n",readUint8(&io));
	printf("serversion:%s\n",readStr(&io));
	printf("threadid:%d\n",readUint32(&io));
	printf("salt1:%lld\n",readUint64(&io));
	printf("zero:%d\n",readUint8(&io));
	printf("capacity:%d\n",readUint16(&io));
	printf("charset:%d\n",readUint8(&io));
	printf("status:%d\n",readUint16(&io));
	printf("capacity:%d\n",readUint16(&io));
	printf("lensalt2:%d\n",readUint8(&io));
	char zero[10];
	readBinary(&io,zero,10);
	printf("pos:%ld\n",vioTell(&io));
	char salt2[13];
	return 0;
}
#endif
