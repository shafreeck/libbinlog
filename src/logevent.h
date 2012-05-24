#ifndef __LOGEVENT_H
#define __LOGEVENT_H
#include <stdint.h>
#include "constant.h"
#include "util.h"

#define LOG_EVENT_HEADER_LEN 19     /* the fixed header length */


#define EVENT_TYPE_OFFSET    4
#define SERVER_ID_OFFSET     5
#define EVENT_LEN_OFFSET     9
#define LOG_POS_OFFSET       13
#define FLAGS_OFFSET         17
enum bl_enum_field_types { MYSQL_TYPE_DECIMAL, MYSQL_TYPE_TINY,
	MYSQL_TYPE_SHORT,  MYSQL_TYPE_LONG,
	MYSQL_TYPE_FLOAT,  MYSQL_TYPE_DOUBLE,
	MYSQL_TYPE_NULL,   MYSQL_TYPE_TIMESTAMP,
	MYSQL_TYPE_LONGLONG,MYSQL_TYPE_INT24,
	MYSQL_TYPE_DATE,   MYSQL_TYPE_TIME,
	MYSQL_TYPE_DATETIME, MYSQL_TYPE_YEAR,
	MYSQL_TYPE_NEWDATE, MYSQL_TYPE_VARCHAR,
	MYSQL_TYPE_BIT,
	MYSQL_TYPE_NEWDECIMAL=246,
	MYSQL_TYPE_ENUM=247,
	MYSQL_TYPE_SET=248,
	MYSQL_TYPE_TINY_BLOB=249,
	MYSQL_TYPE_MEDIUM_BLOB=250,
	MYSQL_TYPE_LONG_BLOB=251,
	MYSQL_TYPE_BLOB=252,
	MYSQL_TYPE_VAR_STRING=253,
	MYSQL_TYPE_STRING=254,
	MYSQL_TYPE_GEOMETRY=255

};
enum bl_log_event_type{
	UNKNOWN_EVENT= 0,
	ROTATE_EVENT= 4,    
	FORMAT_DESCRIPTION_EVENT= 15, 
	TABLE_MAP_EVENT = 19, 
	WRITE_ROWS_EVENT = 23, 
	UPDATE_ROWS_EVENT = 24, 
	DELETE_ROWS_EVENT = 25, 
	ENUM_END_EVENT = 28
};
enum bl_ctypes {
	INT32,
	INT8,
	INT16,
	INT64,
	UINT32,
	UINT64,
	FLOAT,
	DOUBLE,
	STRING,
	BINARY
};
/*Cells structure,type,funcs ...*/
typedef struct cell_st{
	int ctype;
	int mtype;
	int length;
	void *value;
}Cell;
typedef Cell* BlRow;
void freeCell(Cell *c);
/*Just be used in value2Cell now*/
typedef struct eventcursor_st{
	uint8_t *ev;
	int cur;
}EventCursor;


/*Header about*/
typedef struct header_st{
	uint32_t ts;
	uint8_t type;
	uint32_t serverid;
	uint32_t evlen;
	uint32_t nextpos;
	uint16_t flags;
	uint8_t extralen;
	char *extra;
}Header;

/*Fill this structure when parse Table map event*/
typedef struct table_st{
	uint32_t ts;/*timestamp of table map event*/
	uint32_t serverid;
	uint32_t nextpos;
	uint32_t evlen;

	char dbname[256]; /*dbname length in binlog is 1 byte,so 256 will be enough*/
	char tblname[256];
	int  nfields;
	uint64_t tableid;

	uint8_t *types;
	uint8_t *canbenull;
	uint16_t *meta;
}TableEv;

/*Fill this structure when parse Format Description Event (FDE)*/
typedef  struct binlog_st{
	uint32_t blver;/*binlog version*/
	uint8_t hdrlen;/*header length*/
	uint32_t masterid;/*I think masterid is  the serverid of FDE*/
	char server[50];/*server version*/
	char posthdr[29];/*post header length of each type event,now we total have 29 events*/
	char errstr[BL_ERROR_SIZE];/*error message*/

	TableEv table;
}Binlog;
typedef struct rows_event_st{
	uint8_t type;
	uint32_t ts;
	uint32_t evlen;
	uint32_t nextpos;
	uint32_t serverid;
	uint64_t tableid;

	uint32_t nfields; /*Count of fields*/
	uint32_t nrows; /*Count of rows image*/
	BlRow *rows;
	BlRow *rowsold;

}RowsEvent;
/*Rotate Event*/
typedef struct rotate_event_st{
	uint64_t position;
	char fileName[256];/*Just accept 255 bytes of binlog name here*/

}RotateEvent;


/*Parse FormatDescriptionEvent and fill the struct Binlog*/
int parseFDE(Binlog *bl,uint8_t *ev);
/*parse event header*/
int parseHeader(Binlog *bl,Header *header,uint8_t *ev);

int parseTableMapEvent(Binlog *bl,TableEv *tbl,uint8_t *ev);

/*TableEv is a complex structure which includes pointers, we should free it if needed*/
void tblevFreeTableRes(TableEv *tblev);

int parseRotateEvent(Binlog *bl,RotateEvent *rotate,uint8_t *ev);

int parseRowsEvent(Binlog *bl,RowsEvent *rev,uint8_t *ev);
/*RowsEvent is a complex structure contains pointers,we should free it if needed*/
void rowsevFreeRows(RowsEvent *rowsev);

/*A util function to get event type on hand*/
#define getEventType(ev) ((uint8_t)(((uint8_t*)ev)[EVENT_TYPE_OFFSET]))

#endif
