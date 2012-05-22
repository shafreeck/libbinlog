#include "../vio.h"
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void testVioBufferWrite(vio *v){
	char buf[1024];
	vioInitWithBuffer(v,buf,sizeof(buf));
	char *s = "viobuffertest";
	vioWrite(v,s,strlen(s));
	assert(strncmp(s,v->io.buffer.buf,strlen(s)) == 0);
	assert(v->io.buffer.pos == strlen(s));
}
void testVioBufferRead(vio *v){
	int size = sizeof("viobuffertest") - 1;
	char buf[1024]={0};
	memcpy(buf,"viobuffertest",size);
	vioInitWithBuffer(v,buf,sizeof(buf));
	char s[size+1];
	s[size] = '\0';
	vioRead(v,s,size);
	assert(strncmp(s,"viobuffertest",size) == 0);
	assert(v->io.buffer.pos == size);
}
void testVioBufferTell(vio *v){
	v->io.buffer.pos = 10;
	assert(vioBufferTell(v) == 10);
}
void vioBufferTest(vio *v){
	testVioBufferWrite(v);
	testVioBufferRead(v);
	testVioBufferTell(v);
}

/*Test about file*/
void testVioFileWrite(vio *v){
	const char *s="viofiletest";
	char out[sizeof("viofiletest")] = {0};
	int slen = strlen(s);
	FILE *fp = fopen(".tmp","w");
	assert(fp);

	vioInitWithFile(v,fp);
	assert(vioWrite(v,s,slen)==1);
	/*reopen the file to read*/
	fclose(fp);
	fp = fopen(".tmp","r");
	assert(fp);

	assert(fread(out,slen,1,fp)==1);
	fclose(fp);
	assert(remove(".tmp")==0);
	assert(strncmp(s,out,slen)==0);
}
void testVioFileRead(vio *v){
	const char *s="viofiletest";
	char out[sizeof("viofiletest")] = {0};
	int slen = strlen(s);
	/*write to file by raw method*/
	FILE *fp = fopen(".tmp","w");
	assert(fp);
	assert(fwrite(s,slen,1,fp) == 1);
	fclose(fp);
	/*reopen file to read by vioRead*/
	fp = fopen(".tmp","r");
	assert(fp);
	vioInitWithFile(v,fp);
	assert(vioRead(v,out,slen)==1);
	assert(strncmp(s,out,slen)==0);
	/*close and remove file*/
	fclose(fp);
	assert(remove(".tmp")==0);
}
void testVioFileTell(v){
	FILE *fp = fopen(".tmp","w");
	const char *s="viofiletest";
	int slen = strlen(s);
	assert(fwrite(s,slen,1,fp) == 1);
	assert(vioFileTell(v)==slen);
	/*close and remove file*/
	fclose(fp);
	assert(remove(".tmp")==0);
}
void vioFileTest(vio *v){
	testVioFileWrite(v);
	testVioFileRead(v);
	testVioFileTell(v);
}

/*test about NETWORK*/
void testVioNetWrite(vio *v){
	/*use file to fake a network*/
	int fd = open(".tmp",O_WRONLY|O_CREAT|O_TRUNC,0664);
	assert(fd!=-1);
	/*vioWrite test*/
	vioInitWithNet(v,fd);
	const char *s="vionettest";
	char out[64]={0};
	int slen = strlen(s);
	assert(vioWrite(v,s,slen) == 1); // 1 for successful
	assert(close(fd)!=-1);

	/*reopen to read by raw method*/
	fd = open(".tmp",O_RDONLY);
	assert(fd!=-1);
	int total = 0;
	while(slen){
		int nread = read(fd,out+total,slen);
		assert(nread != -1);
		slen -= nread;
		total += nread;
	}

	assert(strncmp(s,out,slen) == 0);
	assert(close(fd)!=-1);
	assert(unlink(".tmp")!=-1);
}
void testVioNetRead(vio *v){
	/*use file to fake a network*/
	int fd = open(".tmp",O_WRONLY|O_CREAT|O_TRUNC,0664);
	assert(fd!=-1);
	/*write by raw method */
	const char *s="vionettest";
	int slen = strlen(s);
	int total = 0;
	while(slen){
		int nwrite = write(fd,s+total,slen);
		assert(nwrite != -1);
		slen -= nwrite;
		total += nwrite;
	}
	assert(close(fd)!=-1);

	/*vioRead test*/
	fd = open(".tmp",O_RDONLY);
	char out[64]={0};
	slen = total;
	vioInitWithNet(v,fd);
	assert(vioRead(v,out,slen)==1);
	assert(strncmp(out,s,slen) == 0);

	/*clean test file*/
	assert(close(fd)!=-1);
	assert(unlink(".tmp")!=-1);
}
void vioNetTest(vio *v){
	testVioNetWrite(v);
	testVioNetRead(v);
}
int main(int argc,char *argv[]){
	vio v;
	vioBufferTest(&v);
	vioFileTest(&v);
	vioNetTest(&v);
	printf("PASS\n");
	return 0;
}
