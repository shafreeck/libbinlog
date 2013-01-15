#include "protocol/protocol.h"
#include "anet.h"
#include "mysql.h"
#include <string.h>
#include <errno.h>
#include <unistd.h>

int mconnect(MySQL *mysql,const char *host,const int port,const char *user,const char *passwd,const char *db){
	int fd = anetTcpConnect(mysql->errstr,(char *)host,port);
	if(fd<0)return 0; 

	mysql->fd = fd;
	PktHeader header;
	readPktHeader(&header,fd);
	mysql->pnum = header.pnum;
	HandshakePkt handshake;
	readHandshakePkt(&handshake,header.plen,fd);
	if(!(handshake.capacity&M_CLIENT_PROTOCOL_41)){
		sprintf(mysql->errstr,"Imcompatible protocol,we use CLIENT_PROTOCOL_41");
		return 0;
	}
	if(handshake.protover!=10){
		sprintf(mysql->errstr,"Unsupport protocol version %d",handshake.protover);
		return 0;
	}
	mysql->scapacity = handshake.capacity;
	mysql->status = handshake.status;

	/*send auth packet*/
	mysql->pnum += 1;
	AuthPkt auth;
	memset(&auth,0,sizeof(auth));
	auth.clientflags = M_CLIENT_PROTOCOL_41;
	auth.clientflags |= M_CLIENT_SECURE_CONNECTION; /*Use new 4.1 authentication*/
	auth.clientflags |= M_CLIENT_LONG_PASSWORD;
	auth.clientflags |= M_CLIENT_LONG_FLAG;
	auth.maxpkt = M_MAX_PACKET_SIZE;
	auth.charset=handshake.charset;
	mysql->maxpkt = auth.maxpkt;
	mysql->ccapacity = auth.clientflags;

	if(user){
		auth.user = (char*)user;
	}
	if(db && handshake.capacity & M_CLIENT_CONNECT_WITH_DB){
		auth.clientflags |= M_CLIENT_CONNECT_WITH_DB;
		auth.database = (char*)db;
	}
	auth.saltlen=0;
	if(passwd){
		auth.saltlen=20;
		cooksalt((unsigned char*)auth.salt,(unsigned char*)handshake.salt,\
				handshake.saltlen,(unsigned char*)passwd,strlen(passwd));
	}

	size_t plen = calcAuthPktLen(&auth);
	writeHeader(plen,mysql->pnum,fd);
	writeAuthPkt(&auth,plen,fd);

	/*try to free if malloced*/
	tryFreeHandshake(&handshake);

	/*Read reply*/
	ReplyPkt reply;
	readPktHeader(&header,fd);
	mysql->pnum = header.pnum;
	readReplyPkt(&reply,header.plen,fd);

	/*Server ask for Old Password Authentication which we do not supported*/
	if(reply.iseof){
		/*Change to mysql_old_password authentication*/
		const int SALTOLD_SIZE = 9;
		char saltold[9]={0};
		mysql->pnum+=1;
		if(passwd){
			cooksaltOld(saltold,handshake.salt,passwd);
		}
		writeHeader(SALTOLD_SIZE,mysql->pnum,fd);
		anetWrite(fd,saltold,SALTOLD_SIZE);
		/*Read reply*/
		readPktHeader(&header,fd);
		mysql->pnum = header.pnum;
		readReplyPkt(&reply,header.plen,fd);
	}

	if(!reply.isok){
		strncpy(mysql->errstr,reply.pkt.err.message,256);
		tryFreeReply(&reply);
		mclose(mysql);
		return 0;
	}else{
		tryFreeReply(&reply);
		mysql->pnum = 0;
		return 1;
	}
}
int mclose(MySQL *mysql){
	if(!mysql)return 0;
	close(mysql->fd);
	mysql->fd = -1;
	mysql->scapacity = 0;
	mysql->ccapacity = 0;
	mysql->threadid = 0;
	mysql->status = 0;
	mysql->charset = 0;
	mysql->pnum = 0;
	return 1;
}

int msendcmd(MySQL *mysql,unsigned char cmd,const char *args,int argslen){
	size_t plen;
	int fd = mysql->fd;
	if(fd<0){
		snprintf(mysql->errstr,128,"Invalid fd, please connnect to mysql first!");
		return 0;
	}
	CmdPkt cpkt;
	cpkt.cmd = cmd;
	cpkt.args = (char *)args;

	plen = argslen + 1;
	if(writeHeader(plen,mysql->pnum,fd)!=PROTO_OK)return 0;
	if(writeCmdPkt(&cpkt,plen,fd)!=PROTO_OK)return 0;
	/*Read reply now*/
	PktHeader header;
	readPktHeader(&header,fd);
	ReplyPkt reply;
	readReplyPkt(&reply,header.plen,fd);
	if(!reply.isok && !reply.iseof){
		snprintf(mysql->errstr,128,"%s",reply.pkt.err.message);
		tryFreeReply(&reply);
		return 0;
	}
	tryFreeReply(&reply);
	return 1;
}
/*realloc if blen is not enough,buf must be on the heap*/
int msaferead(MySQL *mysql,char *buf,size_t blen,char **newbuf){
	int fd = mysql->fd,nread;
	PktHeader header;
	readPktHeader(&header,fd);
	//printf("msaferead:plen %d\n",header.plen);
	//printf("msaferead:pnum %d\n",header.pnum);

    ReplyPkt pkt;
	size_t left = header.plen;
	uint8_t marker;
	if(anetRead(fd,(char*)&marker,1)==-1)
		return -1;
	left -= 1;
	pkt.isok = 1;
	pkt.iseof = 0;
	if(marker == 0xFF){
        pkt.isok = 0;
		pkt.pkt.err.marker = marker;
		readErrorPkt(&(pkt.pkt.err),left,fd);
	}else if(marker == 0xFE && left < 8){
		pkt.pkt.eof.marker = marker;
		pkt.iseof = 1;
		readEofPkt(&(pkt.pkt.eof),left,fd);
    }
    if(!pkt.isok){
        if(!pkt.iseof){
            strncpy(mysql->errstr,pkt.pkt.err.message,256);
        }else{
            snprintf(mysql->errstr,255,"Read EOF packet");
        }
        return -1;
    }

    /*Read the content if it is not a reply packet*/
	if(blen < header.plen && newbuf!=NULL){
		*newbuf = malloc(header.plen) ;
		nread = anetRead(fd,*newbuf,left);
	}else{
		*newbuf = NULL;
		nread = anetRead(fd,buf,left);
	}
	if(nread==-1){
		sprintf(mysql->errstr,"%s",strerror(errno));
	}
	return nread;
}

/*TODO implement these functions
  int minsert(MySQL *mysql,const char *sqlfmt,...);
  int mdelete(MySQL *mysql,const char *sqlfmt,...);
  int mupdate(MySQL *mysql,const char *sqlfmt,...);
  */

