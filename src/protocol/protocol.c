#include "protocol.h"
#include "../anet.h"
#include "sha1.h"
#include <string.h>
#include <math.h>
int readPktHeader(PktHeader *header,int fd){
	char buf[4]={0};
	int nread;
	nread = anetRead(fd,buf,4);
	if(nread==-1)return PROTO_ERR;
	vio io;
	vioInitWithBuffer(&io,buf,4);
	header->plen = readUint24(&io);
	header->pnum = readUint8(&io);
	return PROTO_OK;
}
int readHandshakePkt(HandshakePkt *pkt,size_t len,int fd){
	char *buf = malloc(len);
	char salt1[9],salt2[64];
	int nread,saltlen,saltleft;
	nread = anetRead(fd,buf,len);
	if(nread==-1)return PROTO_ERR;
	vio io;
	vioInitWithBuffer(&io,buf,len);
	pkt->protover = readUint8(&io);
	pkt->mysqlver = readStr(&io);
	pkt->threadid = readUint32(&io);
	readBinary(&io,salt1,9);//NULL-terminated
	memcpy(pkt->salt,salt1,8);
	pkt->saltlen = 8;
	/*May be old protocol just ends here, see sql-common/client.c*/
	if(vioTell(&io)>=len){
		free(buf);
		return PROTO_OK; 
	}

	pkt->capacity = readUint16(&io);
	/*new protocol support, with 16 bytes to describe server characteristics*/
	if(vioTell(&io)+16>len){
		free(buf);
		return PROTO_OK; 
	}
	pkt->charset = readUint8(&io);
	pkt->status = readUint16(&io);
	uint32_t ucapacity = readUint16(&io);
	pkt->capacity |= ucapacity << 16;
	/*saltlen is always 0 up to 5.1, mysql 5.5 use this field with real length */
	saltlen = readUint8(&io);
	(void)saltlen; //anti-waring not-used
	/*10 0x00 bytes ,temp use salt2*/
	readBinary(&io,salt2,10);
	if(saltlen>pkt->saltlen){
		/*plus 1 for '\0'*/
	//	saltleft = saltlen - pkt->saltlen + 1;  /*Ignore this,mysql5.5 saltlen=21 ,but this looks like useless*/
		saltleft = 13;
	}
	else{
		saltleft = 13;
	}//I down know why this does not work for mysql5.5 ,saltlen is useless
	readBinary(&io,salt2,saltleft);
	memcpy(pkt->salt + pkt->saltlen,salt2,saltleft);
	/*ignore the '\0'*/
	pkt->saltlen += saltleft ;

	/*mysql 5.5* support , with plugin name (NULL-terminated)*/
	if(vioTell(&io) < len){
		pkt->pluginname = readStr(&io);
	}else{
		pkt->pluginname = NULL;
	}
	free(buf);
	return PROTO_OK;
}
void tryFreeHandshake(HandshakePkt *pkt){
	free(pkt->mysqlver);
	if(pkt->pluginname)
		free(pkt->pluginname);
}
/*See MySQL_Internals_ClientServer_Protocol . Password functions*/
unsigned char *cooksalt(unsigned char cooked[20],const unsigned char *salt,size_t slen,const unsigned char *passwd,size_t plen){
	SHA1_CTX sha;	
	unsigned char hash1[20],hash2[20];
	int i;

	SHA1Init(&sha);
	SHA1Update(&sha,passwd,plen);
	SHA1Final(hash1,&sha);

	SHA1Init(&sha);
	SHA1Update(&sha,hash1,20);
	SHA1Final(hash2,&sha);

	SHA1Init(&sha);
	SHA1Update(&sha,salt,slen);
	SHA1Update(&sha,hash2,20);
	SHA1Final(hash2,&sha);

	for(i = 0; i < 20; ++i){
		cooked[i] = hash1[i]^hash2[i];
	}
	
	return cooked;
}
/*From mysql5.5.13 mysql_com.h*/
struct rand_struct {
	unsigned long seed1,seed2,max_value;
	double max_value_dbl;
};
/*From mysql5.5.13 password.c*/
void randominit(struct rand_struct *rand_st, unsigned long seed1, unsigned long seed2)
{                                               /* For mysql 3.21.# */
#ifdef HAVE_purify
	bzero((char*) rand_st,sizeof(*rand_st));      /* Avoid UMC varnings */
#endif
	rand_st->max_value= 0x3FFFFFFFL;
	rand_st->max_value_dbl=(double) rand_st->max_value;
	rand_st->seed1=seed1%rand_st->max_value ;
	rand_st->seed2=seed2%rand_st->max_value;
}
/*From mysql5.5.13 password.c*/
double my_rnd(struct rand_struct *rand_st)
{
	rand_st->seed1=(rand_st->seed1*3+rand_st->seed2) % rand_st->max_value;
	rand_st->seed2=(rand_st->seed1+rand_st->seed2+33) % rand_st->max_value;
	return (((double) rand_st->seed1)/rand_st->max_value_dbl);
}
/*From mysql5.5.13 password.c*/
void hash_old_password(unsigned long *result, const char *password, unsigned int password_len)
{
	register unsigned long nr=1345345333L, add=7, nr2=0x12345671L;
	unsigned long tmp;
	const char *password_end= password + password_len;
	for (; password < password_end; password++)
	{
		if (*password == ' ' || *password == '\t')
			continue;                                 /* skip space in password */
		tmp= (unsigned long) (unsigned char) *password;
		nr^= (((nr & 63)+add)*tmp)+ (nr << 8); 
		nr2+=(nr2 << 8) ^ nr; 
		add+=tmp;
	}
	result[0]=nr & (((unsigned long) 1L << 31) -1L); /* Don't use sign bit (str2int) */;
	result[1]=nr2 & (((unsigned long) 1L << 31) -1L);
}
char *cooksaltOld(char cooked[9],const char salt[8],const char *passwd){
	struct rand_struct rand_st;
	const char *salt_end= salt + 8;
	unsigned long hashpass[2];
	unsigned long hashsalt[2];
	if(passwd && passwd[0]){
		char extra, *salt_start=cooked;
		hash_old_password(hashpass,passwd,strlen(passwd));
		hash_old_password(hashsalt,salt,8);
		randominit(&rand_st,hashpass[0] ^ hashsalt[0],
				hashpass[1] ^ hashsalt[1]);
		for (; salt < salt_end; salt++)
			*cooked++= (char) (floor(my_rnd(&rand_st)*31)+64);
		extra=(char) (floor(my_rnd(&rand_st)*31));
		while (salt_start != cooked) 
			*(salt_start++)^=extra;
	}
	*cooked=0;
	return cooked;
}

size_t calcAuthPktLen(const AuthPkt *pkt){
	size_t plen = 4 + 4 + 1 + 23 +strlen(pkt->user) + 1 + 1 + sizeof("mysql_native_password");
	/*saltlen is always 20 so far,so the encoded length occupy 1 byte*/
	if(pkt->saltlen)
		plen += pkt->saltlen;
	if(pkt->database)
		plen += strlen(pkt->database) + 1;
	return plen;
}
/*Client Authentication Packet . Version 4.1+ */
int writeAuthPkt(const AuthPkt *pkt,size_t plen,int fd){
	char *buf = malloc(plen);
	memset(buf,0,plen);
	char zero[23]={0};
	vio io;
	vioInitWithBuffer(&io,buf,plen);
	/*Write clientflags*/
	writeBinary(&io,(const char*)&(pkt->clientflags),4);
	/*Write max packet size*/
	writeBinary(&io,(const char*)&(pkt->maxpkt),4);
	/*Write charset number*/
	writeBinary(&io,(const char*)&(pkt->charset),1);
	/*Write the filled 0*/
	writeBinary(&io,zero,23);
	/*Write username ,include the '\0' */
	writeBinary(&io,pkt->user,strlen(pkt->user)+1);

	/*Write encoded salt len,it is always 20,so the coded len is 1*/
	writeBinary(&io,(const char *)&(pkt->saltlen),1);
	if(pkt->saltlen){
		writeBinary(&io,pkt->salt,pkt->saltlen);
	}

	/*Write database*/
	if(pkt->database){
		writeBinary(&io,pkt->database,strlen(pkt->database)+1);
	}
	//writeBinary(&io,"mysql_native_password",strlen("mysql_native_password")+1);

	/*Now send the packet,send the header first*/
	int nwrite;
	nwrite = anetWrite(fd,buf,plen);

	free(buf);
	if(nwrite < plen) return PROTO_ERR;
	return PROTO_OK;
}
int writeHeader(size_t plen,uint8_t pnum,int fd){
	char buf[4];
	memcpy(buf,&plen,4);
	buf[3] = (char)pnum;
	if(anetWrite(fd,buf,4)==-1)
		return PROTO_ERR;
	return PROTO_OK;
}
static uint64_t readEncodeLength(vio *io){
	uint8_t first = readUint8(io);	
	if(first<251) return first;
	if(first == 251)return 0;/*colume value = NULL, only appropriate in a Row Data Packet*/
	if(first == 252) return readUint16(io);
	if(first == 253) return readUint24(io);
	if(first == 254) return readUint64(io);
	return 0;
}
int readOkPkt(OkPkt *pkt,size_t plen,int fd){
	int nread;
	char *okbuf = malloc(plen);
	if(!okbuf)return PROTO_ERR;
	nread = anetRead(fd,okbuf,plen);
	if(nread<plen){
		free(okbuf);
		return PROTO_ERR;
	}

	vio io;
	vioInitWithBuffer(&io,okbuf,plen);
	pkt->affected = readEncodeLength(&io);
	pkt->insertid = readEncodeLength(&io);
	if(vioTell(&io)==plen) {
		free(okbuf);
		pkt->message = NULL;
		return PROTO_OK;
	}
	/*We assume this is CLIENT_PROTOCOL_41*/
	pkt->status = readUint16(&io);
	pkt->warnings =  readUint16(&io);

	uint64_t slen = plen - vioTell(&io);
	char *msg=NULL;
	if(slen){
		msg = malloc(slen+1);
		readBinary(&io,msg,slen);
		msg[slen]='\0';
	}
	pkt->message = msg;
	free(okbuf);
	return PROTO_OK;
}
int readErrorPkt(ErrorPkt *pkt,size_t plen,int fd){
	int nread,left;
	char *errbuf = malloc(plen);
	if(!errbuf)return PROTO_ERR;
	nread = anetRead(fd,errbuf,plen);
	if(nread<plen){
		free(errbuf);
		return PROTO_ERR;
	}

	vio io;
	vioInitWithBuffer(&io,errbuf,plen);
	pkt->errno = readUint16(&io);
	pkt->smarker = readUint8(&io);
	/*We should ensure this is CLIENT_PROTOCOL_41 to do this*/
	if(pkt->smarker=='#'){
		readBinary(&io,pkt->state,5);
	}
	left = plen - vioTell(&io);
	if(left==0){
		pkt->message = NULL;
		return PROTO_OK;
	}
	pkt->message = malloc(left+1);
	readBinary(&io,pkt->message,left);
	pkt->message[left] = '\0';
	return PROTO_OK;
}
int readEofPkt(EofPkt *pkt,size_t plen,int fd){
	if(plen == 0)return PROTO_OK; // NOT CLIENT_PROTOCOL_41
	int nread;
	char eofbuf[4];
	nread = anetRead(fd,eofbuf,plen);
	if(nread<plen){
		return PROTO_ERR;
	}
	vio io;
	vioInitWithBuffer(&io,eofbuf,plen);
	pkt->warnings = readUint16(&io);
	pkt->status = readUint16(&io);
	return PROTO_OK;
}
int readReplyPkt(ReplyPkt *pkt,size_t plen,int fd){
	size_t left = plen;
	uint8_t marker;
	if(anetRead(fd,(char*)&marker,1)==-1)
		return PROTO_ERR;
	left -= 1;
	pkt->isok = 0;
	pkt->iseof = 0;
	if(marker==0){
		pkt->isok = 1;
		pkt->pkt.ok.marker = marker;
		return readOkPkt(&(pkt->pkt.ok),left,fd);
	}else if(marker == 0xFF){
		pkt->pkt.err.marker = marker;
		return readErrorPkt(&(pkt->pkt.err),left,fd);
	}else if(marker == 0xFE && left < 8){
		// It is an EOF packet
		pkt->pkt.eof.marker = marker;
		pkt->iseof = 1;
		return readEofPkt(&(pkt->pkt.eof),left,fd);
	}
	return PROTO_ERR;
}
void tryFreeReply(ReplyPkt*pkt){
	if(pkt){
		if(pkt->iseof){
			return;
		}
		if(pkt->pkt.err.message && !pkt->isok)
			free(pkt->pkt.err.message);
		if(pkt->pkt.ok.message && pkt->isok)
			free(pkt->pkt.ok.message);
	}
}
int writeCmdPkt(CmdPkt *pkt,size_t plen,int fd){
	int nwrite;
	if(plen<0){
		plen = 1 + strlen(pkt->args);
	}
	char *buf = malloc(plen);
	vio io;
	vioInitWithBuffer(&io,buf,plen);
	writeBinary(&io,(char*)&(pkt->cmd),1);
	writeBinary(&io,pkt->args,plen-1);

	nwrite = anetWrite(fd,buf,plen);
	free(buf);
	if(nwrite==-1)return PROTO_ERR;
	return PROTO_OK;
}

