#ifndef __LIBBINLOG__H
#define __LIBBINLOG__H
#include "logevent.h"
#include "constant.h"
#include "datasource.h"

typedef struct binlog_row_st{
	int nfields;
	int eventType;
	Cell *row;
	Cell *rowOld;
}BinlogRow;

typedef struct binlog_client_st{
	//for private use
	BinlogRow *_rows;
	int _lenRows;

	DataSource *dataSource;
	LogBuffer *logbuffer;
	TableMapEvent *currentTable;
	uint64_t currentTablePosition;
	char *fileName;
	uint32_t index;
	char errstr[BL_ERROR_SIZE];

}BinlogClient;

BinlogRow* fetchOne(BinlogClient *bc);
BinlogClient *connectDataSource(const char*url,uint32_t position,uint32_t index,int serverId);

void printCell(Cell *cell);
#endif
