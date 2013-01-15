#ifndef M_PROTOCOL_H
#define M_PROTOCOL_H
#include "binary.h"

enum Status{
	PROTO_ERR=0,
	PROTO_OK=1
};
typedef struct _pktHeader{
	int32_t plen; //packet length 3 bytes
	uint8_t pnum; //packet number 1 byte
} PktHeader;
typedef struct _handshakePkt{
	uint8_t protover;
	char *mysqlver;
	uint32_t threadid;
	uint8_t saltlen;
	char salt[64]; //assume salt is less the 64 bytes, it is 20 up to mysql5.1*
	uint32_t capacity; // server_capabilities 
	uint8_t charset; //server_language
	uint16_t status;
	char *pluginname;
} HandshakePkt;
int readPktHeader(PktHeader *header,int fd);
int readHandshakePkt(HandshakePkt *pkt,size_t plen,int fd);
/*Free mysqlver and pluginname if needed*/
void tryFreeHandshake(HandshakePkt *pkt);

typedef struct _authPkt{
	uint32_t clientflags;
	uint32_t maxpkt;
	uint8_t charset;
	char *user;
	char saltlen;
	char salt[20];
	char *database;
}AuthPkt;

size_t calcAuthPktLen(const AuthPkt *pkt);
int writeHeader(size_t plen,uint8_t pnum,int fd);
int writeAuthPkt(const AuthPkt *pkt,size_t plen,int fd);

unsigned char *cooksalt(unsigned char cooked[20],const unsigned char *salt,size_t slen,const unsigned char *passwd,size_t plen);
char *cooksaltOld(char cooked[9],const char salt[8],const char *passwd);

struct _cmdpacket{
	uint8_t cmd;
	char *args;
};

typedef struct _okpkt{
	uint8_t marker; /*error packet marker , always 0*/
	uint64_t affected;
	uint64_t insertid;
	uint16_t status;/*Can be used by client to check if we are inside an transaction.*/
	uint16_t warnings ;/* Stored in 2 bytes; New in 4.1 protocol*/
	char *message; /*Encoded length string*/
}OkPkt;
typedef struct _errorpkt{
	uint8_t marker; /*error packer marker, always 0xFF*/
	uint16_t errno;
	uint8_t smarker;/*CLIENT_PROTOCOL_41 support , sql state marker, always '#'*/
	char state[5];/*CLIENT_PROTOCOL_41 support, sqlstate (5 characters)*/
	char *message;
}ErrorPkt;
typedef struct _eofpkt{
	uint8_t marker;
	uint16_t warnings;
	uint16_t status;
}EofPkt;
typedef struct _reply{
	int isok;
	int iseof;

	union{
		OkPkt ok;
		ErrorPkt err;
		EofPkt eof;
	}pkt;
}ReplyPkt;
int readReplyPkt(ReplyPkt*pkt,size_t plen,int fd);
void tryFreeReply(ReplyPkt*pkt);
/*int readOkPkt(OkPkt *pkt,size_t plen,int fd);*/
int readErrorPkt(ErrorPkt *pkt,size_t plen,int fd);
int readEofPkt(EofPkt *pkt,size_t plen,int fd);
typedef struct _cmdpkt{
	uint8_t cmd;
	char *args;
}CmdPkt;
int writeCmdPkt(CmdPkt *pkt,size_t plen,int fd);

#endif
