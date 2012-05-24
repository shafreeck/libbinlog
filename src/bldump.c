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

	BinlogClient *bc = connectDataSource(url,4,0,0);
	if(bc->err){
		printf("%s\n",bc->errstr);
		return 1;
	}
	BinlogRow *row;
	while((row = fetchOne(bc))){
		if(row->type==WRITE_ROWS_EVENT){
			printf("insert ");
		}else if(row->type==UPDATE_ROWS_EVENT){
			printf("update ");
		}else if(row->type == DELETE_ROWS_EVENT){
			printf("delete ");
		}
		int i = 0;
		for(i = 0;i<row->nfields; ++i){
			Cell cell = row->row[i];
			if(cell.value==NULL){
				printf("field:%d\n",i);
				continue;
			}
			printCell(&cell);
		}
		printf("%s %d %d",bc->dataSource->logfile,bc->dataSource->position,bc->dataSource->index);
		printf("\n");
		if(row->type == UPDATE_ROWS_EVENT){
			printf("uptold:");
			for(i = 0;i<row->nfields; ++i){
				Cell cell = row->rowOld[i];
				if(cell.value==NULL){
					printf("field:%d\n",i);
					continue;
				}
				printCell(&cell);
			}
			printf("%s %d %d",bc->dataSource->logfile,bc->dataSource->position,bc->dataSource->index);
			printf("\n");

		}
		freeBinlogRow(row);
	}
	freeBinlogClient(bc);

	return 0;
}
/*
 *getRowEvent()
 *fetchFromBuffer()
 *fetchOne()
 * */
