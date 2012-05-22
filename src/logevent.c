#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "decimal.h"
#include "logevent.h"
#include "util.h"

static uint32_t readUint32(LogBuffer *logbuffer){
	uint32_t v = getUint32(logbuffer->ev+logbuffer->cur);
	logbuffer->cur += 4;
	return v;
}
static uint64_t readUint64(LogBuffer *logbuffer){
	uint64_t v = getUint64(logbuffer->ev + logbuffer->cur);
	logbuffer->cur += 8;
	return v;
}
static uint8_t readUint8(LogBuffer *logbuffer){
	uint8_t v = getUint8(logbuffer->ev + logbuffer->cur);
	logbuffer->cur += 1;
	return v;
}
static uint16_t readUint16(LogBuffer *logbuffer){
	uint16_t v = getUint16(logbuffer->ev + logbuffer->cur);
	logbuffer->cur += 2;
	return v;
}
static uint64_t readUint48(LogBuffer *logbuffer){
	uint64_t v = getUint48(logbuffer->ev + logbuffer->cur);
	logbuffer->cur += 6;
	return v;
}
static uint32_t readUint24(LogBuffer *logbuffer){
	uint32_t v = getUint24(logbuffer->ev + logbuffer->cur);
	logbuffer->cur += 3;
	return v;
}
static int32_t readInt32(LogBuffer *logbuffer){
	int32_t v = getInt32(logbuffer->ev + logbuffer->cur);
	logbuffer->cur += 4;
	return v;
}
static int64_t readInt64(LogBuffer *logbuffer){
	int64_t v = getInt64(logbuffer->ev + logbuffer->cur);
	logbuffer->cur += 8;
	return v;
}
static int8_t readInt8(LogBuffer *logbuffer){
	int8_t v = getInt8(logbuffer->ev + logbuffer->cur);
	logbuffer->cur += 1;
	return v;
}
static int16_t readInt16(LogBuffer *logbuffer){
	int16_t v = getInt16(logbuffer->ev + logbuffer->cur);
	logbuffer->cur += 2;
	return v;
}
static int32_t readInt24(LogBuffer *logbuffer){
	int32_t v = getInt24(logbuffer->ev + logbuffer->cur);
	logbuffer->cur += 3;
	return v;
}
int unpackInteger(LogBuffer *logbuffer){
	uint8_t v = readInt8(logbuffer);
	if(v < 251){
		return v;
	}else if(v == 251){
		return 0;	
	}else if(v== 252){
		return readUint16(logbuffer);
	}else if(v == 253){
		return readUint24(logbuffer);
	}else if(v == 254){
		return readUint64(logbuffer);
	}
	return 0;
}
int isSet(uint8_t *bitmap,int idx){
	int i = idx / 8;	
	int j = idx % 8;
	return (bitmap[i] >> j)& 0x01;
}
static uint16_t* decodeMetadata(int nfields,uint8_t *types, int lenMetadata, uint8_t *metadata){
	uint16_t *meta = (uint16_t*)malloc(nfields*sizeof(uint16_t));
	int i;
	int index = 0;
	for(i=0;i<nfields;++i){
		uint8_t type = types[i];
		switch(type){
			case MYSQL_TYPE_TINY_BLOB:
			case MYSQL_TYPE_BLOB:
			case MYSQL_TYPE_MEDIUM_BLOB:
			case MYSQL_TYPE_LONG_BLOB:
			case MYSQL_TYPE_DOUBLE:
			case MYSQL_TYPE_FLOAT:
			case MYSQL_TYPE_GEOMETRY:
				meta[i] = metadata[index];
				index++;
				break;
			case MYSQL_TYPE_SET:
			case MYSQL_TYPE_ENUM:
			case MYSQL_TYPE_STRING:
			{
				uint16_t x = (metadata[index++] << 8U); // real_type
				x+= metadata[index++];            // pack or field length
				meta[i]= x;
				break;
			}
			case MYSQL_TYPE_BIT:
			{
				uint16_t x= metadata[index++]; 
				x = x + (metadata[index++] << 8U);
				meta[i]= x;
				break;
			}
			case MYSQL_TYPE_VARCHAR:
			{
				char *ptr= (char *)&metadata[index];
				meta[i]= getUint16(ptr);
				index= index + 2;
				break;
			}
			case MYSQL_TYPE_NEWDECIMAL:
			{
				uint16_t x= metadata[index++] << 8U; // precision
				x+= metadata[index++];            // decimals
				meta[i]= x;
				break;
			}
			default:
				meta[i]= 0;
				break;
		}
	}
	return meta;
}

Header *parseEventHeader(LogBuffer *logbuffer){
	if(logbuffer == NULL) return NULL;
	Header* header = createHeader();

	header->timestamp = readUint32(logbuffer);
	header->typeCode = readUint8(logbuffer);
	header->serverId = readUint32(logbuffer);
	header->eventLength = readUint32(logbuffer);
	header->nextPosition = readUint32(logbuffer);
	header->flags = readUint16(logbuffer);

	return header;
}

FormatDescriptionEvent *parseFDE(LogBuffer*logbuffer,Header*header){
	if(logbuffer==NULL || header == NULL) return NULL;
	FormatDescriptionEvent *fde = createFormatDescriptionEvent();

	fde->header = header;
	fde->binlogVersion = readUint16(logbuffer);

	snprintf(fde->serverVersion,50,"%s",logbuffer->ev+logbuffer->cur);
	logbuffer->cur += 50;

	fde->createTimestamp = readUint32(logbuffer);
	fde->headerLength = readUint8(logbuffer);
	int len = header->eventLength - logbuffer->cur;
	fde->postHeaderLengths = (uint8_t*)malloc(len);
	int i;
	for(i = 0 ;i < len ; ++i){
		fde->postHeaderLengths[i] = (uint8_t)logbuffer->ev[i];
	}
	return fde;
}

TableMapEvent *parseTableMapEvent(LogBuffer *logbuffer, Header *header){
	if(logbuffer==NULL || header == NULL) return NULL;
	TableMapEvent *tme = createTableMapEvent();
	tme->header = header;
	tme->tableId = readUint48(logbuffer);
	tme->reserved = readUint16(logbuffer);

	int len = readUint8(logbuffer);
	tme->dbName = (char *)malloc(len+1);
	snprintf(tme->dbName,len+1,"%s",logbuffer->ev + logbuffer->cur);
	logbuffer->cur += len+1; //dbName is null terminated

	len = readUint8(logbuffer);
	tme->tableName = (char*)malloc(len+1);
	snprintf(tme->tableName,len+1,"%s",logbuffer->ev + logbuffer->cur);
	logbuffer->cur += len+1; //tableName is null terminated

	tme->nfields = unpackInteger(logbuffer);
	// types
	tme->fieldTypes = (uint8_t *)malloc(tme->nfields);
	memcpy(tme->fieldTypes,logbuffer->ev+logbuffer->cur,tme->nfields);
	logbuffer->cur += tme->nfields;

	tme->lenMetadata = unpackInteger(logbuffer);
	uint8_t *metadata = (uint8_t *)malloc(tme->lenMetadata);
	memcpy(tme->metadata,logbuffer->ev + logbuffer->cur, tme->lenMetadata);
	logbuffer->cur += tme->lenMetadata;
	tme->metadata = decodeMetadata(tme->nfields,tme->fieldTypes,tme->lenMetadata,metadata);
	free(metadata);

	tme->isNulls = (uint8_t *)malloc((tme->nfields + 7)/8);
	memcpy(tme->isNulls,logbuffer->ev + logbuffer->cur, (tme->nfields+7)/8);
	logbuffer->cur += (tme->nfields+7)/8;

	return tme;
}
RotateEvent *parseRotateEvent(LogBuffer *logbuffer, Header *header){
	if(logbuffer==NULL || header == NULL) return NULL;

	RotateEvent *re = createRotateEvent();
	re->header = header;
	re->position = readUint64(logbuffer);
	int len = header->eventLength - logbuffer->cur;
	re->fileName = (char*)malloc(len+1);
	snprintf(re->fileName,len,"%s",logbuffer->ev + logbuffer->cur);
	re->fileName[len] = 0;
	return re;
}
Cell value2Cell(LogBuffer *logbuffer,uint8_t type,uint32_t meta,int isNull);
RowsEvent *parseRowsEvent(LogBuffer *logbuffer,Header *header,TableMapEvent *tme){
	if(logbuffer==NULL || header == NULL) return NULL;
	RowsEvent *re = createRowsEvent();
	re->header = header;
	re->tableId = readUint48(logbuffer);
	re->reserved = readUint16(logbuffer);
	re->nfields = unpackInteger(logbuffer);
	int size = (re->nfields+7)/8;
	re->isUseds = (uint8_t *)malloc(size);
	memcpy(re->isUseds,logbuffer->ev+logbuffer->cur,size);
	logbuffer->cur += size;
	if(header->typeCode == UPDATE_ROWS_EVENT){
		re->isUsedsUpdate = (uint8_t *)malloc(size); // FIXME
		memcpy(re->isUsedsUpdate,logbuffer->ev+logbuffer->cur,size);
		logbuffer->cur += size;

	}
	uint8_t *isNulls = (uint8_t *)malloc(size);
	list *pairs = listCreate();
	while(logbuffer->cur < logbuffer->evLength){
		RowPair *pair = (RowPair *)malloc(sizeof(RowPair));
		Cell *rold = (Cell *)malloc(sizeof(Cell)*re->nfields);
		Cell *rnew = (Cell *)malloc(sizeof(Cell)*re->nfields);
		memcpy(isNulls,logbuffer->ev+logbuffer->cur,size);
		logbuffer->cur += size;
		int i;
		for(i = 0 ; i < re->nfields; ++i){
			int isUsed = isSet(re->isUseds,i);
			int isNull = isSet(isNulls,i);
			if(!isUsed) continue;
			Cell cell = value2Cell(logbuffer,tme->fieldTypes[i],tme->metadata[i],isNull);
			rold[i] = cell;
		}
		if(header->typeCode == UPDATE_ROWS_EVENT){
			memcpy(isNulls,logbuffer->ev+logbuffer->cur,size);
			logbuffer->cur += size;
			for(i = 0 ; i < re->nfields; ++i){
				int isUsed = isSet(re->isUseds,i);
				int isNull = isSet(isNulls,i);
				if(!isUsed) continue;
				Cell cell = value2Cell(logbuffer,tme->fieldTypes[i],tme->metadata[i],isNull);
				rnew[i] = cell;
			}
		}
		if(header->typeCode == WRITE_ROWS_EVENT){
			pair->rowNew = rold;
			pair->rowOld = NULL;
			free(rnew);
		}else if(header->typeCode == DELETE_ROWS_EVENT){
			pair->rowNew = NULL;
			pair->rowOld = rold;
			free(rnew);
		}else if(header->typeCode == UPDATE_ROWS_EVENT){
			pair->rowNew = rnew;
			pair->rowOld = rold;
		}
		listAddNodeTail(pairs,pair);
	}
	re->pairs = pairs;
	return re;
}
Header *createHeader(){
	Header *header = (Header *)malloc(sizeof(Header));
	header->extraHeaders = NULL;
	return header;
}
FormatDescriptionEvent *createFormatDescriptionEvent(){
	FormatDescriptionEvent *fde = (FormatDescriptionEvent *)malloc(sizeof(FormatDescriptionEvent));
	fde->header = NULL;
	fde->postHeaderLengths = NULL;
	return fde;
}
TableMapEvent *createTableMapEvent(){
	TableMapEvent *tme = (TableMapEvent *)malloc(sizeof(TableMapEvent));
	tme->header = NULL;
	tme->dbName = NULL;
	tme->tableName = NULL;
	tme->fieldTypes = NULL;
	tme->metadata = NULL;
	tme->isNulls = NULL;
	return tme;
}

RotateEvent *createRotateEvent(){
	RotateEvent *rotate = (RotateEvent *)malloc(sizeof(RotateEvent));
	rotate->header = NULL;
	rotate->fileName = NULL;
	return rotate;
}
RowsEvent *createRowsEvent(){
	RowsEvent *rowsev = (RowsEvent *)malloc(sizeof(RowsEvent));
	rowsev->header = NULL;
	rowsev->isUseds = NULL;
	rowsev->isUsedsUpdate = NULL;
	rowsev->pairs = NULL;
	return rowsev;
}
void freeHeader(Header *header){
	if(header->extraHeaders){
		free(header->extraHeaders);
	}
	free(header);
}
void freeFormatDescriptionEvent(FormatDescriptionEvent *fde){
	if(fde->header){
		freeHeader(fde->header);
	}
	if(fde->postHeaderLengths){
		free(fde->postHeaderLengths);
	}
	free(fde);
}
void freeTableMapEvent(TableMapEvent *tme){
	if(tme->header){
		freeHeader(tme->header);
	}
	if(tme->dbName){
		free(tme->dbName);
	}
	if(tme->tableName){
		free(tme->tableName);
	}
	if(tme->fieldTypes){
		free(tme->fieldTypes);	
	}
	if(tme->metadata){
		free(tme->metadata);
	}
	if(tme->isNulls){
		free(tme->isNulls);
	}
}
void freeRotateEvent(RotateEvent *rotate){
	if(rotate->header){
		freeHeader(rotate->header);
	}
	if(rotate->fileName){
		free(rotate->fileName);
	}

}
void freeRowsEvent(RowsEvent *rowsev){
	if(rowsev->header){
		freeHeader(rowsev->header);
	}
	if(rowsev->isUseds){
		free(rowsev->isUseds);
	}
	if(rowsev->isUsedsUpdate){
		free(rowsev->isUsedsUpdate);
	}
	if(rowsev->pairs){
		listRelease(rowsev->pairs);
	}
}

Cell value2Cell(LogBuffer *logbuffer,uint8_t type,uint32_t meta,int isNull){
	Cell cell;
	cell.value = NULL;
	cell.length = 0;
	cell.cppType = cell.mysqlType = 0; //FIXME: add UNKNOW type
	if(isNull){
		return cell;
	}
	uint32_t nLength= 0;
	if (type == MYSQL_TYPE_STRING){
		if (meta >= 256){
			uint8_t byte0= meta >> 8;
			uint8_t byte1= meta & 0xFF;

			if ((byte0 & 0x30) != 0x30) {
				/* a long CHAR() field: see #37426 */
				nLength= byte1 | (((byte0 & 0x30) ^ 0x30) << 4);
				type= byte0 | 0x30;
			}else {
				nLength = meta & 0xFF;
			}
			switch(byte0){
				case MYSQL_TYPE_SET:
				case MYSQL_TYPE_ENUM:
				case MYSQL_TYPE_STRING:
					type= byte0;
					nLength= byte1;
					break;

			}
		}else{
			nLength= meta;
		}
	}
	switch(type){
		case MYSQL_TYPE_LONG: //int32
			{

				cell.mysqlType = MYSQL_TYPE_LONG;
				cell.cppType = INT32;

				uint32_t v = readInt32(logbuffer);

				void *value = malloc(sizeof(int32_t));
				*(int32_t *)value = v;
				cell.value = value;

				cell.length = sizeof(int32_t);
				break;
			}
		case MYSQL_TYPE_TINY:
			{
				int8_t v = readInt8(logbuffer);
				cell.mysqlType = MYSQL_TYPE_TINY;
				cell.cppType = INT8;

				void *value = malloc(sizeof(int8_t));
				*(int8_t*)value = v;
				cell.value = value;
				cell.length = sizeof(int8_t);
				break;
			}
		case MYSQL_TYPE_SHORT:
			{
				int16_t v = readInt16(logbuffer);
				cell.mysqlType = MYSQL_TYPE_SHORT;
				cell.cppType = INT16;
				void *value = malloc(sizeof(int16_t));
				*(int16_t*)value = v;
				cell.value = value;
				cell.length = sizeof(int16_t);
				break;
			}
		case MYSQL_TYPE_LONGLONG:
			{
				int64_t v = readInt64(logbuffer);

				cell.mysqlType = MYSQL_TYPE_LONGLONG;
				cell.cppType = UINT64;
				void *value = malloc(sizeof(int64_t));
				*(int64_t *)value = v;
				cell.value = value;
				cell.length = sizeof(int64_t);
				break;

			}
		case MYSQL_TYPE_INT24:
			{
				int32_t v = readInt24(logbuffer);
				cell.mysqlType = MYSQL_TYPE_INT24;
				cell.cppType = INT32;
				void *value = malloc(sizeof(int32_t));
				*(int32_t *)value = v;
				cell.value = value;
				cell.length = sizeof(int32_t);
				break;
			}
		case MYSQL_TYPE_TIMESTAMP:
			{
				uint32_t v = readUint32(logbuffer);
				cell.mysqlType = MYSQL_TYPE_TIMESTAMP;
				cell.cppType = UINT32;
				void *value = malloc(sizeof(int32_t));
				*(int32_t *)value = v;
				cell.value = value;
				cell.length = sizeof(int32_t);
				break;
			}
		case MYSQL_TYPE_DATETIME:
			{
				uint64_t v = readUint64(logbuffer);
				char szDateTime[128] = {0};
				int32_t d = v/1000000;
				int32_t t = v%1000000;
				sprintf(szDateTime,"%04d-%02d-%02d %02d:%02d:%02d",
						d / 10000, (d % 10000) / 100, d % 100,
						t / 10000, (t % 10000) / 100, t % 100);

				cell.mysqlType = MYSQL_TYPE_DATETIME;
				cell.cppType = STRING;
				cell.value = strdup(szDateTime); // copy '\0' for STRING type
				cell.length = strlen(szDateTime);
				break;
			}

		case MYSQL_TYPE_TIME:
			{
				uint32_t v= readUint24(logbuffer);
				char szTime[128] = {0};
				sprintf(szTime, "'%02d:%02d:%02d'",
						v / 10000, (v % 10000) / 100, v % 100);
				cell.mysqlType = MYSQL_TYPE_TIME;
				cell.cppType = STRING;
				cell.value = strdup(szTime);
				cell.length = strlen(szTime);
				break;
			}

		case MYSQL_TYPE_NEWDATE:
			{
				uint32_t tmp= readUint24(logbuffer);
				int part;
				char buf[11];
				char *pos= &buf[10];  // start from '\0' to the beginning

				/* Copied from field.cc */
				*pos--=0;                 // End NULL
				part=(int) (tmp & 31);
				*pos--= (char) ('0'+part%10);
				*pos--= (char) ('0'+part/10);
				*pos--= ':';
				part=(int) (tmp >> 5 & 15);
				*pos--= (char) ('0'+part%10);
				*pos--= (char) ('0'+part/10);
				*pos--= ':';
				part=(int) (tmp >> 9);
				*pos--= (char) ('0'+part%10); part/=10;
				*pos--= (char) ('0'+part%10); part/=10;
				*pos--= (char) ('0'+part%10); part/=10;
				*pos=   (char) ('0'+part);

				char szNewDate[128] = {0};
				sprintf(szNewDate,"%s",buf);

				cell.mysqlType = MYSQL_TYPE_NEWDATE;
				cell.cppType = STRING;
				cell.value = strdup(szNewDate);
				cell.length = strlen(szNewDate);
				break;
			}

		case MYSQL_TYPE_DATE:
			{
				uint32_t v= readUint24(logbuffer);

				char szDate[128] = {0};
				sprintf(szDate,"%04ld-%02ld-%02ld",
						(v / (16L * 32L)), (v / 32L % 16L), (v % 32L));
				cell.mysqlType = MYSQL_TYPE_DATE;
				cell.cppType = STRING;
				cell.value = strdup(szDate);
				cell.length = strlen(szDate);
				break;
			}

		case MYSQL_TYPE_YEAR:
			{
				uint32_t v = readUint8(logbuffer);
				char szYear[8];
				sprintf(szYear, "%04d", v+ 1900);
				cell.mysqlType = MYSQL_TYPE_YEAR;
				cell.cppType = STRING;
				cell.value = strdup(szYear);
				cell.length = strlen(szYear);
				break;
			}

		case MYSQL_TYPE_ENUM:
			{
				cell.mysqlType =  MYSQL_TYPE_ENUM;
				cell.cppType = INT32;
				switch (meta & 0xFF) {
					case 1:
						{
							uint32_t v = readUint8(logbuffer);
							void *value = malloc(sizeof(uint8_t));
							*(uint32_t *)value = v;
							cell.value = value;
							cell.length = sizeof(uint32_t);
							break;
						}
					case 2:
						{
							uint32_t v = readUint16(logbuffer);
							void *value = malloc(sizeof(uint8_t));
							*(uint32_t *)value = v;
							cell.value = value;
							cell.length = sizeof(uint32_t);
							break;
						}
					default:
						break;
				}
				break;
			}

		case MYSQL_TYPE_SET:
			{
				char *szBuf =(char *) malloc(32);
				memcpy(szBuf,logbuffer->ev,meta & 0xFF);
				//*pPos += meta & 0xFF;
				cell.mysqlType = MYSQL_TYPE_SET;
				cell.cppType = BINARY;
				cell.value = szBuf;
				cell.length = meta & 0xFF;
				logbuffer->cur += meta & 0xFF;
				break;
			}
		case MYSQL_TYPE_BLOB:
			cell.mysqlType = MYSQL_TYPE_BLOB;
			cell.cppType = BINARY;
			switch (meta) {
				case 1:     //TINYBLOB/TINYTEXT
					{
						nLength = readUint8(logbuffer);
						void *value = malloc(nLength);
						memcpy((char*)value,logbuffer->ev+logbuffer->cur,nLength);
						cell.value = value;
						cell.length = nLength;
						logbuffer->cur += nLength;
						break;
					}
				case 2:     //BLOB/TEXT
					{
						nLength = readUint16(logbuffer);
						void *value = malloc(nLength);
						memcpy((char *)value,logbuffer->ev+logbuffer->cur, nLength);
						cell.value = value;
						cell.length = nLength;
						logbuffer->cur += nLength;
						break;
					}
				case 3:     //MEDIUMBLOB/MEDIUMTEXT
					{
						//   printf("MEDIUMBLOB/MEDIUMTEXT\r\n");
						nLength = readUint24(logbuffer);
						void *value = malloc(nLength);
						memcpy((char *)value,logbuffer->ev+logbuffer->cur, nLength);
						cell.value = value;
						cell.length = nLength;
						logbuffer->cur += nLength;
						break;
					}
				case 4:     //LONGBLOB/LONGTEXT
					{
						//    printf("LONGBLOB/LONGTEXT\r\n");
						nLength = readUint32(logbuffer);
						void *value = malloc(nLength);
						memcpy((char *)value,logbuffer->ev+logbuffer->cur, nLength);
						cell.value = value;
						cell.length = nLength;
						logbuffer->cur += nLength;
						break;
					}
				default:
					break;
			}
			break;

		case MYSQL_TYPE_VARCHAR:
		case MYSQL_TYPE_VAR_STRING:
			{
				cell.mysqlType = MYSQL_TYPE_VARCHAR;
				cell.cppType = STRING; // can we store binary into VARCHAR ?
				nLength = meta;
				if (nLength < 256){
					nLength = readUint8(logbuffer);
					void *value = malloc(nLength+1); // store '\0' for STRING type
					memcpy((char *)value,logbuffer->ev+logbuffer->cur, nLength );
					((char *)value)[nLength] = '\0';
					cell.value = value;
					cell.length = nLength;
					logbuffer->cur += nLength;
				}else{
					nLength = readUint16(logbuffer);
					void *value = malloc(nLength+1); // store '\0' for STRING type
					memcpy((char *)value,logbuffer->ev+logbuffer->cur, nLength );
					((char *)value)[nLength] = '\0';
					cell.value = value;
					cell.length = nLength;
					logbuffer->cur += nLength;
				}
				break;
			}

		case MYSQL_TYPE_STRING:
			{
				cell.mysqlType = MYSQL_TYPE_STRING;
				cell.cppType = STRING;
				if (nLength < 256){
					nLength = readUint8(logbuffer);
					void *value = malloc(nLength+1);
					memcpy((char *)value,logbuffer->ev+logbuffer->cur, nLength );
					((char *)value)[nLength] = '\0';
					cell.value = value;
					cell.length = nLength;
					logbuffer->cur += nLength;
				}else{
					nLength = readUint16(logbuffer);
					void *value = malloc(nLength+1);
					memcpy((char *)value,logbuffer->ev+logbuffer->cur, nLength );
					((char *)value)[nLength] = '\0';
					cell.value = value;
					cell.length = nLength;
					logbuffer->cur += nLength;
				}
				break;

			}

		case MYSQL_TYPE_BIT:
			{
				cell.cppType = BINARY;
				cell.mysqlType = MYSQL_TYPE_BIT;
				uint32_t nBits= ((meta >> 8) * 8) + (meta & 0xFF);
				nLength= (nBits + 7) / 8;
				void *value = malloc(nLength);
				memcpy((char *)value,logbuffer->ev+logbuffer->cur, nLength );
				cell.value = value;
				cell.length = nLength;
				logbuffer->cur += nLength;
				break;
			}   
		case MYSQL_TYPE_FLOAT:
			{
				cell.mysqlType = MYSQL_TYPE_FLOAT;
				cell.cppType = FLOAT;
				void *value = malloc(sizeof(float));
				memcpy((char *)value,logbuffer->ev+logbuffer->cur, sizeof(float) );
				cell.value = value;
				cell.length = sizeof(float);
				logbuffer->cur += sizeof(float);
				break;
			}   
		case MYSQL_TYPE_DOUBLE:
			{
				cell.mysqlType = MYSQL_TYPE_DOUBLE;
				cell.cppType = DOUBLE;
				void *value = malloc(sizeof(double));
				memcpy((char *)value,logbuffer->ev+logbuffer->cur, sizeof(double) );
				cell.value = value;
				cell.length = sizeof(double);
				logbuffer->cur += sizeof(double);
				break;

			}
		case MYSQL_TYPE_NEWDECIMAL:// FIXME : check this type
			{
				uint8_t precision = meta>>8;
				uint8_t decimals = meta & 0xFF;
				uint32_t size = decimal_bin_size(precision,decimals);
				decimal_t d;
				bin2decimal(logbuffer->ev+logbuffer->cur,&d,precision,decimals);
				char buf[512] = {0};
				int buflen = 512;
				decimal2string(&d,buf,&buflen,precision,decimals,0);
				cell.mysqlType = MYSQL_TYPE_NEWDECIMAL;
				cell.cppType = STRING;
				cell.value = strdup(buf);
				cell.length = buflen;
				logbuffer->cur += size;
				cell.length = strlen(buf);
			}

		default:
			break;
	}
	return cell;
}
