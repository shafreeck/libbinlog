#include "../binary.h"
#include <assert.h>
#include <limits.h>
/*hack to set position to HEAD*/
static void resetBufferPos(vio *v){
	v->io.buffer.pos = 0;
}
void testReadBinary(vio *v){
	int val = 10;
	vioWrite(v,(char *)&val,sizeof(val));
	resetBufferPos(v);
	char buf[sizeof(int)] = {0};
	readBinary(v,buf,sizeof(int));
	assert(memcmp(buf,&val,sizeof(int))==0);
}
void testReadInt32(vio *v){
	resetBufferPos(v);
	uint32_t val = UINT_MAX;
	vioWrite(v,(char *)&val,sizeof(val));
	resetBufferPos(v);

	int32_t nval = readInt32(v);
	assert(-1 == nval);
}
void testReadInt24(vio *v){
	resetBufferPos(v);
	int32_t val = INT_MAX & 0x00FFFFFF;
	vioWrite(v,(char *)&val,sizeof(val));
	resetBufferPos(v);
	char buf[3]={0};
	int32_t nval = readInt24(v);
	assert(nval == -1);
}

void testReadUint24(vio *v){
	resetBufferPos(v);
	int32_t val = INT_MAX & 0x00FFFFFF;
	vioWrite(v,(char *)&val,sizeof(val));
	resetBufferPos(v);
	char buf[3]={0};
	int32_t nval = readUint24(v);
	assert(nval == val);
}
void testBinary(vio *v){
	testReadBinary(v);
	testReadInt32(v);
	testReadInt24(v);
	testReadUint24(v);
}

int main(int argc, const char *argv[]) {
	vio v;
	char buf[1024] = {0};
	vioInitWithBuffer(&v,(char *)buf,1024);
	testBinary(&v);
	
	printf("PASS\n");
	return 0;
}
