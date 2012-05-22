#ifndef __LOGEVENT_H
#define __LOGEVENT_H
#include <stdint.h>
#include "adlist.h"

#define LOG_EVENT_HEADER_LEN 19     /* the fixed header length */


#define EVENT_TYPE_OFFSET    4
#define SERVER_ID_OFFSET     5
#define EVENT_LEN_OFFSET     9
#define LOG_POS_OFFSET       13
#define FLAGS_OFFSET         17
enum enum_field_types { MYSQL_TYPE_DECIMAL, MYSQL_TYPE_TINY,
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
enum log_event_type{
	UNKNOWN_EVENT= 0,
	ROTATE_EVENT= 4,    
	FORMAT_DESCRIPTION_EVENT= 15, 
	TABLE_MAP_EVENT = 19, 
	WRITE_ROWS_EVENT = 23, 
	UPDATE_ROWS_EVENT = 24, 
	DELETE_ROWS_EVENT = 25, 
	ENUM_END_EVENT = 28
};
enum cpp_types {
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
typedef struct header_st{
	uint32_t timestamp;
	uint8_t typeCode;
	uint32_t serverId;
	uint32_t eventLength;
	uint32_t nextPosition;
	uint16_t flags;
	void *extraHeaders;
}Header;
typedef struct logbuffer_st{
	uint8_t *ev;
	int cur;
	uint32_t evLength;
	char errstr[128];

}LogBuffer;

typedef struct format_description_event_st{
	Header *header;
	uint32_t binlogVersion;
	char serverVersion[50];
	uint32_t createTimestamp;
	uint32_t headerLength;
	uint8_t* postHeaderLengths; //FIXME : replace by array

}FormatDescriptionEvent;

typedef struct table_map_event_st{
	Header *header;
	uint64_t tableId;
	uint16_t reserved;
	char *dbName;
	char *tableName;
	uint32_t nfields;
	uint8_t *fieldTypes;
	uint32_t lenMetadata;
	uint16_t *metadata;
	uint8_t *isNulls;
}TableMapEvent;

typedef struct rotate_event_st{
	Header *header;
	uint64_t position;
	char *fileName;

}RotateEvent;

typedef struct rows_event_st{
	Header *header;
	uint64_t tableId;
	uint16_t reserved;
	uint32_t nfields;
	uint8_t *isUseds;
	uint8_t *isUsedsUpdate;
	list *pairs; // a list of RowPair

}RowsEvent;
/*
   typedef struct update_rows_event_st{
   Header *header;
   uint8_t *isUseds;
   uint8_t *isNulls;
   list *pairs; // a list of RowPair

   }UpdateRowsEvent;

   typedef struct Delete_rows_event_st{
   Header *header;
   list *pairs; // a list of RowPair
   uint8_t *isUseds;
   uint8_t *isNulls;
   list *rows;
   }DeleteRowsEvent;
   */
typedef struct cell_st{
	int cppType;
	int mysqlType;
	int length;
	void *value;
}Cell;
typedef struct row_pair_st{
	Cell* rowNew;
	Cell* rowOld;
}RowPair;

Header *parseEventHeader(LogBuffer *logbuffer);
FormatDescriptionEvent *parseFDE(LogBuffer*logbuffer,Header*header);
TableMapEvent *parseTableMapEvent(LogBuffer *logbuffer, Header *header);
RotateEvent *parseRotateEvent(LogBuffer *logbuffer, Header *header);
RowsEvent *parseRowsEvent(LogBuffer *logbuffer,Header *header,TableMapEvent *tme);

Header *createHeader();
FormatDescriptionEvent *createFormatDescriptionEvent();
TableMapEvent *createTableMapEvent();
RotateEvent *createRotateEvent();
RowsEvent *createRowsEvent();

void freeHeader(Header *h);
void freeFormatDescriptionEvent(FormatDescriptionEvent *fde);
void freeTableMapEvent(TableMapEvent *tme);
void freeRotateEvent(RotateEvent *rotate);
void freeRowsEvent(RowsEvent *rowsev);

#endif
