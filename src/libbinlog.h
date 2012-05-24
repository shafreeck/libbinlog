#ifndef __LIBBINLOG__H
#define __LIBBINLOG__H
#include "logevent.h"
#include "constant.h"
#include "datasource.h"

typedef struct binlog_row_st{
	int nfields;
	int type;
	int created; /*binlog event created  timestamp*/
	Cell *row;
	Cell *rowOld;
}BinlogRow;

typedef struct binlog_client_st{
	//for private use
	BinlogRow *_rows;
	int _lenRows;
	//end private

	Binlog binlog;
	DataSource *dataSource;
	int  err;
	char errstr[BL_ERROR_SIZE];

}BinlogClient;

BinlogClient *connectDataSource(const char*url,uint32_t position,uint32_t index,int serverId);
BinlogRow* fetchOne(BinlogClient *bc);
void freeBinlogRow(BinlogRow *row);
void freeBinlogClient(BinlogClient *bc);

void printCell(Cell *cell);
#endif
