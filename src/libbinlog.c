#include "libbinlog.h"
#include "util.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void printCell(Cell *cell){
	switch(cell->ctype){
		case INT32:
			printf("%d\t",getInt32(cell->value));
			break;
		case UINT32:
			printf("%d\t",getUint32(cell->value));
			break;
		case INT64:
			printf("%ld\t",getInt64(cell->value));
			break;
		case UINT64:
			printf("%lu\t",getUint64(cell->value));
			break;
		case INT8:
			printf("%d",getInt8(cell->value));
			break;
		case INT16:
			printf("%d",getInt16(cell->value));
			break;
		case FLOAT:
			printf("%f\t",getFloat(cell->value));
			break;
		case DOUBLE:
			printf("%f\t",getDouble(cell->value));
			break;
		case STRING: //MySQL type BINARY may be STRING
			{   
				char *value =(char *) cell->value;
				printf("%s\t",(char*)value);
				break;

			}   
		case BINARY:
			printf("BINARY\t");
			break;
		default:
			printf("UNKNOW\t");
			break;

	}
}
static int getRowsEvent(BinlogClient *bc,RowsEvent *rev){
	for(;;){
		void *ev = dsGetEvent(bc->dataSource);
		if(ev == NULL){
			bc->err = 1;
			sprintf(bc->errstr,bc->dataSource->errstr,255);
			break;
		}

		uint8_t evtype = getEventType(ev);
		if(evtype==BL_FORMAT_DESCRIPTION_EVENT){
			parseFDE(&(bc->binlog),ev);
		}
		if(evtype==BL_TABLE_MAP_EVENT){
			TableEv *tblev = &(bc->binlog.table);
			parseTableMapEvent(&(bc->binlog),&(bc->binlog.table),ev);
			bc->dataSource->position = tblev->nextpos - tblev->evlen;
			bc->dataSource->index = 0;

		}else if(evtype==BL_WRITE_ROWS_EVENT || evtype==BL_UPDATE_ROWS_EVENT || evtype==BL_DELETE_ROWS_EVENT){
			parseRowsEvent(&(bc->binlog),rev,ev);
			dsFreeEvent(bc->dataSource);
			return 1;
		}else if(evtype==BL_ROTATE_EVENT){
			RotateEvent rotate;
			parseRotateEvent(&(bc->binlog),&rotate,ev);
			bc->dataSource->position = rotate.position;
			strcpy(bc->dataSource->logfile,rotate.fileName);
			//TODO record the log file name
		}
		dsFreeEvent(bc->dataSource);
	}
	/*anti warning*/
	return 0;
}
static void setRowsEventBuffer(BinlogClient *bc,RowsEvent *ev){
	/*We are first Here,so malloc enough memory*/
	if(bc->_rows==NULL && bc->_lenRows < ev->nrows){
		bc->_rows = (BinlogRow *)malloc(ev->nrows * sizeof(BinlogRow));
	}
	/*If bc->_rows size is not enough , reallocate*/
	if(bc->_rows && bc->_lenRows<ev->nrows){
		/*Free rows array we allocated  last time*/
		free(bc->_rows);
		bc->_rows = (BinlogRow *)malloc(ev->nrows * sizeof(BinlogRow));
	}
	bc->_cur = 0;
	int nfields = ev->nfields;
	bc->_lenRows = ev->nrows;
	int i = 0;
	while(i < ev->nrows){
		BinlogRow row;
		row.nfields = nfields;
		row.type = ev->type;
		row.created = ev->ts;
		row.rowOld = NULL;
		row.row = ev->rows[i];
		if(row.type == BL_UPDATE_ROWS_EVENT){
			row.rowOld = ev->rowsold[i];
		}
		bc->_rows[i++] = row;
		//printf("bc->rows[0] :%p,row:%p\n",&(bc->_rows[0]),&row);
	}
	/*Free rows array here,for we copy all rows to bc->_rows[]*/
	free(ev->rows);
	if(ev->type==BL_UPDATE_ROWS_EVENT){
		free(ev->rowsold);
	}
}
static BinlogRow* fetchFromBuffer(BinlogClient *bc){
	if(bc->_cur < bc->_lenRows){
		BinlogRow *row = &(bc->_rows[bc->_cur]);
		bc->_cur+=1;
		return row;
	}else{
		return NULL;
	}
}
// ****** public interface ******
void freeBinlogRow(BinlogRow *br){
	if(!br)return;
	int i;
	for(i=0; i < br->nfields; ++i){
		freeCell(&(br->row[i]));	
		if(br->type==BL_UPDATE_ROWS_EVENT){
			freeCell(&(br->rowOld[i]));	
		}
	}
	free(br->row);
	if(br->type==BL_UPDATE_ROWS_EVENT){
		free(br->rowOld);
	}

}
BinlogRow* _fetchOne(BinlogClient *bc){
	if(bc->_cur == bc->_lenRows){
		RowsEvent rowsev;
		int ret = getRowsEvent(bc,&rowsev);	
		if(ret == 0){
			snprintf(bc->errstr,BL_ERROR_SIZE,"%s",bc->dataSource->errstr);
			bc->dataSource->index = 0;
			return NULL;
		}
		setRowsEventBuffer(bc,&rowsev);
	}
	/*Incr index , this can not be place begin of _fetchOne
	 *Because when something wrong, getRowsEvent return 0, _fetchOne will return NULL
	 *at this time , we should not incr index otherwise we give an error index when shutdown!
	 * */
	bc->dataSource->index ++;
	return fetchFromBuffer(bc);
}
BinlogRow * fetchOne(BinlogClient *bc){
	BinlogRow *row=NULL;
	int start = bc->_startidx;
	while(bc->_startidx){ //Skip if start index is not 0
		uint32_t oldpos = bc->dataSource->position;
		printf("oldpos:%d\n",oldpos);
		row = _fetchOne(bc);
		bc->_startidx -= 1;
		/*Wow ,  It is a long story
		 * All we need to do this because we introduce the concept 'index'
		 * the index incr when one of the two conditions match below:
		 *  1. One rows event consist of multiple row-images
		 *  2. Multiple rows event after one table map event 
		 * If the index is specified when connectDataSource, we seek to the Table map event 
		 * and skip all the index which less than the start index
		 *
		 * However, if someone specify an index which is invaild,we should know that . 
		 * We check about the position of Table map event . If we change to next table map event
		 * and the index is still less than the start index, we assume that the start index is
		 * valid
		 *
		 * */
		if(oldpos!=bc->dataSource->position){
			snprintf(bc->errstr,BL_ERROR_SIZE,"Impossible index %d at position %d",start,oldpos);
			bc->err=1;
			return NULL;
		}
	}
	row = _fetchOne(bc);
	return row;
}
BinlogClient *connectDataSource(const char *url,uint32_t position, uint32_t index,int serverid){
	BinlogClient *bc =(BinlogClient *)malloc(sizeof(BinlogClient));
	/*Must init before use,may be we can supply a wrapper*/
	memset(bc,0,sizeof(BinlogClient));

	DataSource *ds = (DataSource*)malloc(sizeof(DataSource));
	if(strncmp(url,"mysql",sizeof("mysql")-1) == 0){
		dsInitWithMySQL(ds,url);
	}
	else if(strncmp(url,"file",sizeof("file")-1) ==0){
		dsInitWithFile(ds,url);
	}
	else{
		snprintf(bc->errstr,BL_ERROR_SIZE,"%s","Unknow  datasource");
		bc->err=1;
		return bc;
	}

	ds->position = position;
	ds->index = 0;
	bc->_startidx = index;
	ds->serverid = serverid;
	if(!dsConnect(ds)){
		snprintf(bc->errstr,BL_ERROR_SIZE,"%s",ds->errstr);
		bc->err=1;
		return bc;
	}

	bc->dataSource = ds;
	return bc;
}

void freeBinlogClient(BinlogClient *bc){
	if(!bc)return;

	if(bc->dataSource){
		dsClose(bc->dataSource);
		free(bc->dataSource);
		bc->dataSource = NULL;
	}
	if(bc->_rows){
		free(bc->_rows);
		bc->dataSource = NULL;
	}
	free(bc);
	bc = NULL;
}

