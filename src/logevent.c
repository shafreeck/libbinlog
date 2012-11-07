#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "decimal.h"
#include "logevent.h"
#include "util.h"
/*EventCousor reader*/
static uint32_t readUint32(EventCursor *evcur){
	uint32_t v = getUint32(evcur->ev+evcur->cur);
	evcur->cur += 4;
	return v;
}
static uint64_t readUint64(EventCursor *evcur){
	uint64_t v = getUint64(evcur->ev + evcur->cur);
	evcur->cur += 8;
	return v;
}
static uint8_t readUint8(EventCursor *evcur){
	uint8_t v = getUint8(evcur->ev + evcur->cur);
	evcur->cur += 1;
	return v;
}
static uint16_t readUint16(EventCursor *evcur){
	uint16_t v = getUint16(evcur->ev + evcur->cur);
	evcur->cur += 2;
	return v;
}

/*nonused  now ,anti warning
static uint64_t readUint48(EventCursor *evcur){
	uint64_t v = getUint48(evcur->ev + evcur->cur);
	evcur->cur += 6;
	return v;
}*/
static uint32_t readUint24(EventCursor *evcur){
	uint32_t v = getUint24(evcur->ev + evcur->cur);
	evcur->cur += 3;
	return v;
}
static int32_t readInt32(EventCursor *evcur){
	int32_t v = getInt32(evcur->ev + evcur->cur);
	evcur->cur += 4;
	return v;
}
static int64_t readInt64(EventCursor *evcur){
	int64_t v = getInt64(evcur->ev + evcur->cur);
	evcur->cur += 8;
	return v;
}
static int8_t readInt8(EventCursor *evcur){
	int8_t v = getInt8(evcur->ev + evcur->cur);
	evcur->cur += 1;
	return v;
}
static int16_t readInt16(EventCursor *evcur){
	int16_t v = getInt16(evcur->ev + evcur->cur);
	evcur->cur += 2;
	return v;
}
static int32_t readInt24(EventCursor *evcur){
	int32_t v = getInt24(evcur->ev + evcur->cur);
	evcur->cur += 3;
	return v;
}

/*parse mysql coded length*/
static uint64_t decodeint(uint8_t *ev ,uint32_t *cur){
	uint8_t v = getInt8(ev);
	*cur += 1;
	if(v < 251) return v;
	if(v == 251) return 0;	
	if(v == 252){ *cur += 2; return getUint16(ev + 1); }
	if(v == 253){ *cur += 3; return getUint24(ev + 1); }
	if(v == 254){ *cur += 8; return getUint64(ev + 1); }
	return 0;
}
/*Check is set on 'idx' bit or not*/
int isSet(uint8_t *bitmap,int idx){
	int i = idx / 8;	
	int j = idx % 8;
	return (bitmap[i] >> j)& 0x01;
}
/*parse Table map event meta info,we change the variable len to fix 2 bytes*/
static uint16_t* decodeMetadata(int nfields,uint8_t *types, int lenMetadata, uint8_t *metadata){
	uint16_t *meta = (uint16_t*)malloc(nfields*sizeof(uint16_t));
	int i;
	int index = 0;
	for(i=0;i<nfields;++i){
		uint8_t type = types[i];
		switch(type){
			case BL_MYSQL_TYPE_TINY_BLOB:
			case BL_MYSQL_TYPE_BLOB:
			case BL_MYSQL_TYPE_MEDIUM_BLOB:
			case BL_MYSQL_TYPE_LONG_BLOB:
			case BL_MYSQL_TYPE_DOUBLE:
			case BL_MYSQL_TYPE_FLOAT:
			case BL_MYSQL_TYPE_GEOMETRY:
				meta[i] = metadata[index];
				index++;
				break;
			case BL_MYSQL_TYPE_SET:
			case BL_MYSQL_TYPE_ENUM:
			case BL_MYSQL_TYPE_STRING:
				{
					uint16_t x = (metadata[index++] << 8U); // real_type
					x+= metadata[index++];            // pack or field length
					meta[i]= x;
					break;
				}
			case BL_MYSQL_TYPE_BIT:
				{
					uint16_t x= metadata[index++]; 
					x = x + (metadata[index++] << 8U);
					meta[i]= x;
					break;
				}
			case BL_MYSQL_TYPE_VARCHAR:
				{
					char *ptr= (char *)&metadata[index];
					meta[i]= getUint16(ptr);
					index= index + 2;
					break;
				}
			case BL_MYSQL_TYPE_NEWDECIMAL:
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
/*Get a cell of one row in (UPDATE,WRITE,DELETE)Rows event*/
Cell value2Cell(EventCursor *evcur,uint8_t type,uint32_t meta){
	Cell cell;
	cell.value = NULL;
	cell.length = 0;
	cell.ctype = cell.mtype = 0; 

	uint32_t nLength= 0;
	if (type == BL_MYSQL_TYPE_STRING){
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
				case BL_MYSQL_TYPE_SET:
				case BL_MYSQL_TYPE_ENUM:
				case BL_MYSQL_TYPE_STRING:
					type= byte0;
					nLength= byte1;
					break;

			}
		}else{
			nLength= meta;
		}
	}
	switch(type){
		case BL_MYSQL_TYPE_LONG: //int32
			{

				cell.mtype = BL_MYSQL_TYPE_LONG;
				cell.ctype = INT32;

				uint32_t v = readInt32(evcur);

				void *value = malloc(sizeof(int32_t));
				*(int32_t *)value = v;
				cell.value = value;

				cell.length = sizeof(int32_t);
				break;
			}
		case BL_MYSQL_TYPE_TINY:
			{
				int8_t v = readInt8(evcur);
				cell.mtype = BL_MYSQL_TYPE_TINY;
				cell.ctype = INT8;

				void *value = malloc(sizeof(int8_t));
				*(int8_t*)value = v;
				cell.value = value;
				cell.length = sizeof(int8_t);
				break;
			}
		case BL_MYSQL_TYPE_SHORT:
			{
				int16_t v = readInt16(evcur);
				cell.mtype = BL_MYSQL_TYPE_SHORT;
				cell.ctype = INT16;
				void *value = malloc(sizeof(int16_t));
				*(int16_t*)value = v;
				cell.value = value;
				cell.length = sizeof(int16_t);
				break;
			}
		case BL_MYSQL_TYPE_LONGLONG:
			{
				int64_t v = readInt64(evcur);

				cell.mtype = BL_MYSQL_TYPE_LONGLONG;
				cell.ctype = INT64;
				void *value = malloc(sizeof(int64_t));
				*(int64_t *)value = v;
				cell.value = value;
				cell.length = sizeof(int64_t);
				break;

			}
		case BL_MYSQL_TYPE_INT24:
			{
				int32_t v = readInt24(evcur);
				cell.mtype = BL_MYSQL_TYPE_INT24;
				cell.ctype = INT32;
				void *value = malloc(sizeof(int32_t));
				*(int32_t *)value = v;
				cell.value = value;
				cell.length = sizeof(int32_t);
				break;
			}
		case BL_MYSQL_TYPE_TIMESTAMP:
			{
				uint32_t v = readUint32(evcur);
				cell.mtype = BL_MYSQL_TYPE_TIMESTAMP;
				cell.ctype = UINT32;
				void *value = malloc(sizeof(int32_t));
				*(int32_t *)value = v;
				cell.value = value;
				cell.length = sizeof(int32_t);
				break;
			}
		case BL_MYSQL_TYPE_DATETIME:
			{
				uint64_t v = readUint64(evcur);
				char szDateTime[128] = {0};
				int32_t d = v/1000000;
				int32_t t = v%1000000;
				sprintf(szDateTime,"%04d-%02d-%02d %02d:%02d:%02d",
						d / 10000, (d % 10000) / 100, d % 100,
						t / 10000, (t % 10000) / 100, t % 100);

				cell.mtype = BL_MYSQL_TYPE_DATETIME;
				cell.ctype = STRING;
				cell.value = strdup(szDateTime); // copy '\0' for STRING type
				cell.length = strlen(szDateTime);
				break;
			}

		case BL_MYSQL_TYPE_TIME:
			{
				uint32_t v= readUint24(evcur);
				char szTime[128] = {0};
				sprintf(szTime, "'%02d:%02d:%02d'",
						v / 10000, (v % 10000) / 100, v % 100);
				cell.mtype = BL_MYSQL_TYPE_TIME;
				cell.ctype = STRING;
				cell.value = strdup(szTime);
				cell.length = strlen(szTime);
				break;
			}

		case BL_MYSQL_TYPE_NEWDATE:
			{
				uint32_t tmp= readUint24(evcur);
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

				cell.mtype = BL_MYSQL_TYPE_NEWDATE;
				cell.ctype = STRING;
				cell.value = strdup(szNewDate);
				cell.length = strlen(szNewDate);
				break;
			}

		case BL_MYSQL_TYPE_DATE:
			{
				uint32_t v= readUint24(evcur);

				char szDate[128] = {0};
				sprintf(szDate,"%04ld-%02ld-%02ld",
						(v / (16L * 32L)), (v / 32L % 16L), (v % 32L));
				cell.mtype = BL_MYSQL_TYPE_DATE;
				cell.ctype = STRING;
				cell.value = strdup(szDate);
				cell.length = strlen(szDate);
				break;
			}

		case BL_MYSQL_TYPE_YEAR:
			{
				uint32_t v = readUint8(evcur);
				char szYear[8];
				sprintf(szYear, "%04d", v+ 1900);
				cell.mtype = BL_MYSQL_TYPE_YEAR;
				cell.ctype = STRING;
				cell.value = strdup(szYear);
				cell.length = strlen(szYear);
				break;
			}

		case BL_MYSQL_TYPE_ENUM:
			{
				cell.mtype =  BL_MYSQL_TYPE_ENUM;
				cell.ctype = INT32;
				switch (meta & 0xFF) {
					case 1:
						{
							uint32_t v = readUint8(evcur);
							void *value = malloc(sizeof(uint8_t));
							*(uint32_t *)value = v;
							cell.value = value;
							cell.length = sizeof(uint32_t);
							break;
						}
					case 2:
						{
							uint32_t v = readUint16(evcur);
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

		case BL_MYSQL_TYPE_SET:
			{
				char *szBuf =(char *) malloc(32);
				memcpy(szBuf,evcur->ev,meta & 0xFF);
				//*pPos += meta & 0xFF;
				cell.mtype = BL_MYSQL_TYPE_SET;
				cell.ctype = BINARY;
				cell.value = szBuf;
				cell.length = meta & 0xFF;
				evcur->cur += meta & 0xFF;
				break;
			}
		case BL_MYSQL_TYPE_BLOB:
			cell.mtype = BL_MYSQL_TYPE_BLOB;
			cell.ctype = BINARY;
			switch (meta) {
				case 1:     //TINYBLOB/TINYTEXT
					{
						nLength = readUint8(evcur);
						void *value = malloc(nLength);
						memcpy((char*)value,evcur->ev+evcur->cur,nLength);
						cell.value = value;
						cell.length = nLength;
						evcur->cur += nLength;
						break;
					}
				case 2:     //BLOB/TEXT
					{
						nLength = readUint16(evcur);
						void *value = malloc(nLength);
						memcpy((char *)value,evcur->ev+evcur->cur, nLength);
						cell.value = value;
						cell.length = nLength;
						evcur->cur += nLength;
						break;
					}
				case 3:     //MEDIUMBLOB/MEDIUMTEXT
					{
						//   printf("MEDIUMBLOB/MEDIUMTEXT\r\n");
						nLength = readUint24(evcur);
						void *value = malloc(nLength);
						memcpy((char *)value,evcur->ev+evcur->cur, nLength);
						cell.value = value;
						cell.length = nLength;
						evcur->cur += nLength;
						break;
					}
				case 4:     //LONGBLOB/LONGTEXT
					{
						//    printf("LONGBLOB/LONGTEXT\r\n");
						nLength = readUint32(evcur);
						void *value = malloc(nLength);
						memcpy((char *)value,evcur->ev+evcur->cur, nLength);
						cell.value = value;
						cell.length = nLength;
						evcur->cur += nLength;
						break;
					}
				default:
					break;
			}
			break;

		case BL_MYSQL_TYPE_VARCHAR:
		case BL_MYSQL_TYPE_VAR_STRING:
			{
				cell.mtype = BL_MYSQL_TYPE_VARCHAR;
				cell.ctype = STRING; // can we store binary into VARCHAR ?
				nLength = meta;
				if (nLength < 256){
					nLength = readUint8(evcur);
					void *value = malloc(nLength+1); // store '\0' for STRING type
					memcpy((char *)value,evcur->ev+evcur->cur, nLength );
					((char *)value)[nLength] = '\0';
					cell.value = value;
					cell.length = nLength;
					evcur->cur += nLength;
				}else{
					nLength = readUint16(evcur);
					void *value = malloc(nLength+1); // store '\0' for STRING type
					memcpy((char *)value,evcur->ev+evcur->cur, nLength );
					((char *)value)[nLength] = '\0';
					cell.value = value;
					cell.length = nLength;
					evcur->cur += nLength;
				}
				break;
			}

		case BL_MYSQL_TYPE_STRING:
			{
				cell.mtype = BL_MYSQL_TYPE_STRING;
				cell.ctype = STRING;
				if (nLength < 256){
					nLength = readUint8(evcur);
					void *value = malloc(nLength+1);
					memcpy((char *)value,evcur->ev+evcur->cur, nLength );
					((char *)value)[nLength] = '\0';
					cell.value = value;
					cell.length = nLength;
					evcur->cur += nLength;
				}else{
					nLength = readUint16(evcur);
					void *value = malloc(nLength+1);
					memcpy((char *)value,evcur->ev+evcur->cur, nLength );
					((char *)value)[nLength] = '\0';
					cell.value = value;
					cell.length = nLength;
					evcur->cur += nLength;
				}
				break;

			}

		case BL_MYSQL_TYPE_BIT:
			{
				cell.ctype = BINARY;
				cell.mtype = BL_MYSQL_TYPE_BIT;
				uint32_t nBits= ((meta >> 8) * 8) + (meta & 0xFF);
				nLength= (nBits + 7) / 8;
				void *value = malloc(nLength);
				memcpy((char *)value,evcur->ev+evcur->cur, nLength );
				cell.value = value;
				cell.length = nLength;
				evcur->cur += nLength;
				break;
			}   
		case BL_MYSQL_TYPE_FLOAT:
			{
				cell.mtype = BL_MYSQL_TYPE_FLOAT;
				cell.ctype = FLOAT;
				void *value = malloc(sizeof(float));
				memcpy((char *)value,evcur->ev+evcur->cur, sizeof(float) );
				cell.value = value;
				cell.length = sizeof(float);
				evcur->cur += sizeof(float);
				break;
			}   
		case BL_MYSQL_TYPE_DOUBLE:
			{
				cell.mtype = BL_MYSQL_TYPE_DOUBLE;
				cell.ctype = DOUBLE;
				void *value = malloc(sizeof(double));
				memcpy((char *)value,evcur->ev+evcur->cur, sizeof(double) );
				cell.value = value;
				cell.length = sizeof(double);
				evcur->cur += sizeof(double);
				break;

			}
		case BL_MYSQL_TYPE_NEWDECIMAL:// FIXME : check this type
			{
				uint8_t precision = meta>>8;
				uint8_t decimals = meta & 0xFF;
				uint32_t size = decimal_bin_size((int)precision,(int)decimals);
				int32_t dbuf[128]; //actually , this is  big enough
				decimal_t d;
				d.buf=dbuf;
				d.len=sizeof(dbuf)/sizeof(dbuf[0]);
				int ret = bin2decimal(evcur->ev+evcur->cur,&d,precision,decimals);
				if(ret!=E_DEC_OK){
					return cell;
				}
				char buf[512] = {0};
				int buflen = sizeof(buf);
				decimal2string(&d,buf,&buflen,0,0,0);
				buf[buflen]='\0';
				cell.mtype = BL_MYSQL_TYPE_NEWDECIMAL;
				cell.ctype = STRING;
				cell.value = strdup(buf);
				cell.length = buflen;
				evcur->cur += size;
			}

		default:
			break;
	}
	return cell;
}
int parseHeader(Binlog *bl,Header *header,uint8_t *ev){
	struct rawheader{
		char ts[4];
		char type;
		char serverid[4];
		char evlen[4];
		char nextpos[4];
		char flags[2];
		char extra[];
	}*rawhdr =  (struct rawheader *)ev;
	header->ts = getUint32(rawhdr->ts);
	header->type = (uint8_t)(rawhdr->type);
	header->serverid = getUint32(rawhdr->serverid);
	header->evlen = getUint32(rawhdr->evlen);
	header->nextpos = getUint32(rawhdr->nextpos);
	header->flags = getUint16(rawhdr->flags);
	header->extralen =  bl->hdrlen - 19;
	header->extra = rawhdr->extra;
	return 1;
}
/*Parse FormatDescriptionEvent and fill the struct Binlog*/
int parseFDE(Binlog *bl,uint8_t *ev){
	struct rawfde{
		char tshdr[4];/*header timestamp*/
		char type;
		char serverid[4];
		char evlen[4];
		char nextpos[4];
		char flags[2];/*end header*/
		char blver[2];
		char server[50];
		char ts[4];
		char hdrlen[1];
		char posthdr[];
	}*fde = (struct rawfde*)ev;
	bl->blver = getUint16(fde->blver);
	bl->hdrlen = getUint8(fde->hdrlen);
	bl->masterid = getUint32(fde->serverid);
	memcpy(bl->server,fde->server,50);
	int evlen = getUint32(fde->evlen);
	assert((evlen-sizeof(struct rawfde)<=29));
	memcpy(bl->posthdr,fde->posthdr,29);
	return 1;
}
int parseTableMapEvent(Binlog *bl,TableEv *tbl,uint8_t *ev){
	uint32_t cur=0;
	struct rawheader{
		char ts[4];
		char type;
		char serverid[4];
		char evlen[4];
		char nextpos[4];
		char flags[2];
		char extra[];
	}*rawhdr =  (struct rawheader *)ev;
	cur += bl->hdrlen;
	struct rawtbl_fixpart{
		char tableid[6];
		char reserved[2];
	}*rawfix = (struct rawtbl_fixpart*)(ev + cur);
	cur += 8;
	struct rawstr{
		uint8_t len;
		char str[];
	} *dbname =(struct rawstr*)(ev + cur);
	cur += dbname->len + 1 + 1;

	struct rawstr *tblname =(struct rawstr*)(ev + cur);
	cur += tblname->len + 1 + 1;
	/*malloc of metadata and types, this should be freed  by parseRowsEvent*/
	uint32_t nfields;
	nfields =(uint32_t) decodeint(ev+cur,&cur);

	/*This should be freed  by parseRowsEvent when not used*/
	uint8_t *types = malloc(nfields);
	memcpy(types,ev+cur,nfields);
	cur += nfields;
	uint32_t  metalen;
	metalen =(uint32_t) decodeint(ev+cur,&cur);
	uint8_t *meta = ev+cur;;
	cur += nfields;

	uint8_t *canbenull=malloc(nfields); 
	memcpy(canbenull,ev+cur,nfields);
	cur += nfields;
	/*fill values to TableEv */
	tbl->ts = getUint32(rawhdr->ts);
	tbl->serverid = getUint32(rawhdr->serverid);
	tbl->nextpos  = getUint32(rawhdr->nextpos);
	tbl->evlen  = getUint32(rawhdr->evlen);

	strcpy(tbl->dbname,dbname->str);
	strcpy(tbl->tblname,tblname->str);
	tbl->nfields = nfields;
	tbl->tableid = getUint48(rawfix->tableid);

	tbl->types = types;
	tbl->canbenull = canbenull;
	/*decode meta info here,from variable length to fix 16bytes*/
	tbl->meta = decodeMetadata(nfields,types,metalen,meta);
	/*free nonused meta,we will use tbl->meta after*/
	return 1;
}
void tblevFreeTableRes(TableEv *tblev){
	free(tblev->meta);
	free(tblev->types);
	free(tblev->canbenull);
}
int parseRotateEvent(Binlog *bl,RotateEvent *rotate,uint8_t *ev){
	uint32_t cur = bl->hdrlen;
	rotate->position =  getUint64(ev+cur);
	cur += 8;
	uint32_t left = getUint32(ev+9) - cur;
	memcpy(rotate->fileName,ev+cur,left);
	rotate->fileName[left]='\0';
	return 1;
}
int parseRowsEvent(Binlog *bl,RowsEvent *rev,uint8_t *ev){
	uint32_t cur = 0;
	Header hdr;
	parseHeader(bl,&hdr,ev);
	cur += bl->hdrlen;
	uint64_t tableid = getUint48(ev+cur);
	cur += 8; // ignore reserved 2 bytes

	rev->ts = hdr.ts;
	rev->type = hdr.type;
	rev->nextpos = hdr.nextpos;
	rev->serverid = hdr.serverid;
	rev->tableid = tableid;
	assert(rev->tableid==bl->table.tableid);

	uint32_t nfields;
	nfields =(uint32_t) decodeint(ev+cur,&cur);
	assert(nfields==bl->table.nfields);
	uint8_t *isused = ev+cur,*isused_u=NULL;
	int bitlen = (int) (nfields+7)/8;
	cur += bitlen;
	if(hdr.type==BL_UPDATE_ROWS_EVENT){
		isused_u = ev+cur;
		cur += bitlen; 
	}
	/*Decode the row images*/
	int i,nrows=0;
	Cell* cells,*cellsnew;
	int totalrows = 8;
	BlRow *rows,*rowsold;
	rows = rowsold = NULL;
	rows = malloc(sizeof(BlRow)*totalrows);
	if(hdr.type==BL_UPDATE_ROWS_EVENT)
		rowsold = malloc(sizeof(BlRow)*totalrows);
	EventCursor evcur;
	evcur.ev = ev;
	evcur.cur = cur;
	while(evcur.cur < hdr.evlen){
		if(nrows >= totalrows){
			totalrows *= 2;
			rows = realloc(rows,sizeof(BlRow)*totalrows);
			rowsold = realloc(rows,sizeof(BlRow)*totalrows);
		}
		uint8_t *isnull = ev + evcur.cur;
		evcur.cur += bitlen; 
		cells = malloc(nfields*sizeof(Cell));
		for(i = 0; i < nfields; ++i){
			if(!isSet(isnull,i) && isSet(isused,i)){
				cells[i] = value2Cell(&evcur,bl->table.types[i],bl->table.meta[i]);
			}else{
				Cell c = {0};
				cells[i] = c;
			}
		}
		rows[nrows] = cells;

		if(hdr.type==BL_UPDATE_ROWS_EVENT){
			/*If the event is BL_UPDATE_ROWS_EVENT , the first row-image is removed
			 * and the second row-image is inserted, so , exchange them*/
			rowsold[nrows] = rows[nrows];
			cellsnew = malloc(nfields*sizeof(Cell));
			isnull = ev + evcur.cur;
			evcur.cur += bitlen; 
			for(i = 0; i < nfields; ++i){
				if(!isSet(isnull,i) && isSet(isused_u,i)){
					cellsnew[i] = value2Cell(&evcur,bl->table.types[i],bl->table.meta[i]);
				}else{
					Cell c = {0};
					cellsnew[i] = c;
				}
			}
			rows[nrows] = cellsnew;
		}
		++nrows;
	}

	rev->rows = rows;
	rev->rowsold = rowsold;
	rev->nrows = nrows;
	rev->nfields = nfields;
	/*clean up Table map event malloced*/
	tblevFreeTableRes(&(bl->table));
	return 1;
}
void rowsevFreeRows(RowsEvent *rowsev){
	int i,j;
	/*Free all rows*/
	for(i = 0;i < rowsev->nrows ; ++i){
		/*Free all cells's value*/
		for(j = 0; j < rowsev->nfields;++j){
			free(rowsev->rows[i][j].value);
		}
		free(rowsev->rows[i]);
	}
	free(rowsev->rows);
	if(rowsev->type==BL_UPDATE_ROWS_EVENT){
		for(i = 0;i < rowsev->nrows ; ++i){
			for(j = 0; j < rowsev->nfields;++j){
				free(rowsev->rowsold[i][j].value);
			}
			free(rowsev->rowsold[i]);
		}
		free(rowsev->rowsold);
	}
}
/*Free the value of a cell*/
void freeCell(Cell *c){
	free(c->value);
}
