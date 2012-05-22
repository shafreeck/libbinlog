#include "libbinlog.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "filedriver.h"
#include "netdriver.h"

int32_t to_int32(void *value) { return *(int32_t*)value ;}
uint32_t to_uint32(void *value) { return *(uint32_t*)value; }
int64_t to_int64(void *value) { return *(int64_t*)value ;}
uint64_t to_uint64(void *value) { return *(uint64_t*)value; }
int8_t to_int8(void *value) { return *(int8_t*)value; }
int16_t to_int16(void *value) { return *(int16_t*)value; }
float to_float(void *value){return *(float *)value;}
double to_double(void *value){return *(double *)value;}
void printCell(Cell *cell){
	switch(cell->cppType){
		case INT32:
			printf("%d\t",to_int32(cell->value));
			break;
		case UINT32:
			printf("%d\t",to_uint32(cell->value));
			break;
		case INT64:
			printf("%lld\t",to_int64(cell->value));
			break;
		case UINT64:
			printf("%llu\t",to_uint64(cell->value));
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
				printf("%d:%s\t",cell->mysqlType,(char*)value);
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
static RowsEvent *getRowsEvent(BinlogClient *bc){
	TableMapEvent *tme = NULL;
	for(;;){
		void *ev = dsGetEvent(bc->dataSource);
		if(ev == NULL)break;
		LogBuffer *s = bc->logbuffer;
		s->ev = ev;
		s->cur = 0;
		Header *h = parseEventHeader(s);
		s->evLength = h->eventLength - 19; // notics ,we must set it!!! //TODO 19 should be instead by header length

		if(h->typeCode==TABLE_MAP_EVENT){
			if(tme!=NULL){
				freeTableMapEvent(tme);
			}
			tme = parseTableMapEvent(s,h);
			if(tme==NULL){
				sprintf(bc->errstr,"%s",s->errstr);
				goto error;
			}
			bc->currentTable = tme;

		}else if(h->typeCode==WRITE_ROWS_EVENT || h->typeCode==UPDATE_ROWS_EVENT || h->typeCode==DELETE_ROWS_EVENT){
			if(tme == NULL){
				goto error;
			}
			RowsEvent *re = parseRowsEvent(s,h,tme);
			/*printf("------------------------\n");
			printf("typeCode:%d\n",re->header->typeCode);
			printf("eventLength:%d\n",re->header->eventLength);
			printf("tableID:%lld\n",re->tableId);
			printf("nfields:%d\n",re->nfields);
			printf("rowsCount:%d\n",listLength(re->pairs));
			printf("------------------------\n");*/
			dsFreeEvent(bc->dataSource);
			return re;
		}
error:
		dsFreeEvent(bc->dataSource);
		continue;
	}
	/*anti warning*/
	return NULL;
}
static void setRowsEventBuffer(BinlogClient *bc,RowsEvent *ev){
	int nfields = bc->currentTable->nfields;
	listIter *iter = listGetIterator(ev->pairs,AL_START_HEAD);
	listNode *node = NULL;
	bc->index = 0;
	bc->_lenRows = listLength(ev->pairs);
	bc->_rows = (BinlogRow *)malloc(bc->_lenRows * sizeof(BinlogRow));
	int i = 0;
	//printf("bc->_lenRows is :%d\n",listLength(ev->pairs));
	while((node=listNext(iter))){
		BinlogRow row;
		row.nfields = nfields;
		row.eventType = ev->header->typeCode;
		row.rowOld = NULL;
		if(row.eventType == WRITE_ROWS_EVENT || row.eventType == UPDATE_ROWS_EVENT){
			row.row = ((RowPair*)(node->value))->rowNew;
		}else if(row.eventType==DELETE_ROWS_EVENT){
			row.row = ((RowPair*)(node->value))->rowOld;
		}
		if(row.eventType == UPDATE_ROWS_EVENT){
			row.rowOld = ((RowPair*)(node->value))->rowOld;
		}
		bc->_rows[i++] = row;
		//printf("bc->rows[0] :%p,row:%p\n",&(bc->_rows[0]),&row);
	}
}
static BinlogRow* fetchFromBuffer(BinlogClient *bc){
	if(bc->index < bc->_lenRows){
		return &(bc->_rows[bc->index]);
	}else{
		return NULL;
	}
}
// ****** public ******
void freeBinlogRow(BinlogRow *br){
	if(br){
		free(br);
	}
}
BinlogRow* fetchOne(BinlogClient *bc){
	bc->index ++;
	if(bc->index>= bc->_lenRows){
		/*free rows buffer malloced last time*/
		if(bc->_rows){
			free(bc->_rows);
		}
		RowsEvent *re = getRowsEvent(bc);	
		if(re == NULL){
			snprintf(bc->errstr,BL_ERROR_SIZE,"%s",bc->dataSource->errstr);
			return NULL;
		}
		setRowsEventBuffer(bc,re);
	}
	return fetchFromBuffer(bc);
}
BinlogClient *connectDataSource(const char *url,uint32_t position, uint32_t index,int serverid){
	BinlogClient *bc =(BinlogClient *)malloc(sizeof(BinlogClient));
	bc->_rows=NULL;
	bc->_lenRows=0;

	DataSource *ds = (DataSource*)malloc(sizeof(DataSource));
	const char *fileName = NULL;
	if(strncmp(url,"mysql",sizeof("mysql")-1) == 0){
		dsInitWithMySQL(ds,url);
	}
	else if(strncmp(url,"file",sizeof("file")-1) ==0){
		dsInitWithFile(ds,url);
	}
	else{
		snprintf(bc->errstr,BL_ERROR_SIZE,"%s","Unknow  datasource");
		return bc;
	}

	ds->position = position;
	ds->index = index;
	ds->serverid = serverid;
	if(!dsConnect(ds)){
		snprintf(bc->errstr,BL_ERROR_SIZE,"%s","Unknow  datasource");
		return bc;
	}
	bc->dataSource = ds;
	LogBuffer *buffer = (LogBuffer*)malloc(sizeof(LogBuffer));
	bc->logbuffer = buffer;

	bc->currentTablePosition = position;
	bc->index = index;
	bc->fileName = (char*)fileName;
	return bc;
}

void freeBinlogClient(BinlogClient *bc){
	if(!bc)return;
	if(bc->_rows){
		free(bc->_rows);
		bc->_rows = NULL;
	}

	if(bc->dataSource){
		free(bc->dataSource);
		bc->dataSource = NULL;
	}
	if(bc->logbuffer){
		free(bc->logbuffer);
		bc->logbuffer = NULL;
	}
	free(bc);
	bc = NULL;
}

