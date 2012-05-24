#include "libbinlog.h"
#include "util.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int32_t to_int32(void *value) { return *(int32_t*)value ;}
uint32_t to_uint32(void *value) { return *(uint32_t*)value; }
int64_t to_int64(void *value) { return *(int64_t*)value ;}
uint64_t to_uint64(void *value) { return *(uint64_t*)value; }
int8_t to_int8(void *value) { return *(int8_t*)value; }
int16_t to_int16(void *value) { return *(int16_t*)value; }
float to_float(void *value){return *(float *)value;}
double to_double(void *value){return *(double *)value;}
void printCell(Cell *cell){
	switch(cell->ctype){
		case INT32:
			printf("%d\t",to_int32(cell->value));
			break;
		case UINT32:
			printf("%d\t",to_uint32(cell->value));
			break;
		case INT64:
			printf("%ld\t",to_int64(cell->value));
			break;
		case UINT64:
			printf("%lu\t",to_uint64(cell->value));
			break;
		case INT8:
			printf("%d",to_int8(cell->value));
			break;
		case INT16:
			printf("%d",to_int16(cell->value));
			break;
		case FLOAT:
			printf("%f\t",to_float(cell->value));
			break;
		case DOUBLE:
			printf("%f\t",to_double(cell->value));
			break;
		case STRING:
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
		if(ev == NULL)break;

		uint8_t evtype = getEventType(ev);
		if(evtype==FORMAT_DESCRIPTION_EVENT){
			parseFDE(&(bc->binlog),ev);
		}
		if(evtype==TABLE_MAP_EVENT){
			TableEv *tblev = &(bc->binlog.table);
			parseTableMapEvent(&(bc->binlog),&(bc->binlog.table),ev);
			bc->dataSource->position = tblev->nextpos - tblev->evlen;
			bc->dataSource->index = 0;

		}else if(evtype==WRITE_ROWS_EVENT || evtype==UPDATE_ROWS_EVENT || evtype==DELETE_ROWS_EVENT){
			parseRowsEvent(&(bc->binlog),rev,ev);
			dsFreeEvent(bc->dataSource);
			return 1;
		}else if(evtype==ROTATE_EVENT){
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
	int nfields = ev->nfields;
	bc->_lenRows = ev->nrows;
	int i = 0;
//	printf("bc->_lenRows is :%d\n",ev->nrows);
	while(i < ev->nrows){
		BinlogRow row;
		row.nfields = nfields;
		row.type = ev->type;
		row.rowOld = NULL;
		row.row = ev->rows[i];
		if(row.type == UPDATE_ROWS_EVENT){
			row.rowOld = ev->rowsold[i];
		}
		bc->_rows[i++] = row;
		//printf("bc->rows[0] :%p,row:%p\n",&(bc->_rows[0]),&row);
	}
	/*Free rows array here,for we copy all rows to bc->_rows[]*/
	free(ev->rows);
	if(ev->type==UPDATE_ROWS_EVENT){
		free(ev->rowsold);
	}
}
static BinlogRow* fetchFromBuffer(BinlogClient *bc){
	if(bc->dataSource->index < bc->_lenRows){
		return &(bc->_rows[bc->dataSource->index]);
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
		if(br->type==UPDATE_ROWS_EVENT){
			freeCell(&(br->rowOld[i]));	
		}
	}
	free(br->row);
	if(br->type==UPDATE_ROWS_EVENT){
		free(br->rowOld);
	}

}
BinlogRow* fetchOne(BinlogClient *bc){
	bc->dataSource->index ++;
	if(bc->dataSource->index>= bc->_lenRows){
		RowsEvent rowsev;
		int ret = getRowsEvent(bc,&rowsev);	
		if(ret == 0){
			snprintf(bc->errstr,BL_ERROR_SIZE,"%s",bc->dataSource->errstr);
			return NULL;
		}
		setRowsEventBuffer(bc,&rowsev);
	}
	return fetchFromBuffer(bc);
}
BinlogClient *connectDataSource(const char *url,uint32_t position, uint32_t index,int serverid){
	BinlogClient *bc =(BinlogClient *)malloc(sizeof(BinlogClient));
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
	ds->index = index;
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

