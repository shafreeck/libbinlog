#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "libbinlog.h"
#include "logevent.h"
#include <assert.h>

int main(int argc,char *argv[]){
	if(argc!=2){
		fprintf(stderr,"Usage:%s <url>\n",argv[0]);
		return 1;
	}
	const char *url = argv[1];

	BinlogClient *bc = connectDataSource(url,4,0,10);
	if(!bc)return 1;
	BinlogRow *row;
	while(1){
		row = fetchOne(bc);
		if(!row){
			return 1;
		}
		int i = 0;
		for(i = 0;i<row->nfields; ++i){
			Cell cell = row->row[i];
			printCell(&cell);
		}
		printf("\n");
	}
	freeBinlogClient(bc);

	return 0;
}
/*
 *getRowEvent()
 *fetchFromBuffer()
 *fetchOne()
 * */
