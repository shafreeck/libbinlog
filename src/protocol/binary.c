#include <string.h>
#include "binary.h"

void readBinary(vio *v,char *buf,size_t len){
	assert(vioRead(v,buf,len));
}
void writeBinary(vio *v,const char *buf,size_t len){
	//assert(vioWrite(v,buf,len)>=0);
	vioWrite(v,buf,len);
}
/*This can be only used in buffer. It's a hack*/
char *readStr(vio *v){
	char *s = strdup(v->io.buffer.buf + v->io.buffer.pos);
	v->io.buffer.pos += strlen(s)+1;
	return s;
}

int8_t readInt8(vio *v){
	char buf[sizeof(int8_t)] = {0};
	readBinary(v,buf,sizeof(uint8_t));
	return *((int8_t*)buf);
}

uint8_t readUint8(vio *v){
	char buf[sizeof(uint8_t)] = {0};
	readBinary(v,buf,sizeof(uint8_t));
	return *((uint8_t *)buf);
}

int16_t readInt16(vio *v){
	char buf[sizeof(int16_t)] = {0};
	readBinary(v,buf,sizeof(int16_t));
	return *((int16_t *)buf);
}

uint16_t readUint16(vio *v){
	char buf[sizeof(uint16_t)] = {0};
	readBinary(v,buf,sizeof(uint16_t));
	return *((uint16_t *)buf);
}

int32_t readInt24(vio *v){
	char buf[3] = {0};
	readBinary(v,buf,3);
	return ((uint32_t) ((uint8_t)buf[0])) + (((uint32_t) ((uint8_t)buf[1]))<<8)+ (((int32_t)((int8_t)buf[2]))<<16);
}

uint32_t readUint24(vio *v){
	char buf[3] = {0};
	readBinary(v,buf,3);
	return ((uint32_t) ((uint8_t)buf[0])) + (((uint32_t) ((uint8_t)buf[1]))<<8)+ (((uint32_t)((uint8_t)buf[2]))<<16);
}

int32_t readInt32(vio *v){
	char buf[sizeof(int32_t)] = {0};
	readBinary(v,buf,sizeof(int32_t));
	return *((int32_t *)buf);
}

uint32_t readUint32(vio *v){
	char buf[sizeof(uint32_t)] = {0};
	readBinary(v,buf,sizeof(uint32_t));
	return *((uint32_t *)buf);
}
int64_t readInt64(vio *v){
	char buf[sizeof(int64_t)] = {0};
	readBinary(v,buf,sizeof(int64_t));
	return *((int64_t *)buf);
}

uint64_t readUint64(vio *v){
	char buf[sizeof(uint64_t)] = {0};
	readBinary(v,buf,sizeof(uint64_t));
	return *((uint64_t *)buf);
}
