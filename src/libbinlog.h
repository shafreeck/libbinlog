#ifndef BL_LIBBINLOG__H
#define BL_LIBBINLOG__H
#include "logevent.h"
#include "constant.h"
#include "datasource.h"

#ifdef __cplusplus
extern "C"{
#endif
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

	/*public member*/
	Binlog binlog;
	DataSource *dataSource;
	int  err;
	char errstr[BL_ERROR_SIZE];

}BinlogClient;

/*Connect to datasource which can be mysql server and local file
 * Param:
 *  url: mysql://user:password@host:port/binlog or file://path-to-binlog
 *  position: start position
 *  index: index number of position
 *  serverid: serverid of this slave , only works for mysql server
 * Return:
 *  BinlogClient pointer ,which impossible be NULL, check BinlogClient->err for error
 * */
BinlogClient *connectDataSource(const char*url,uint32_t position,uint32_t index,int serverId);

/*Fetch one row from binlog. It can be inserted ,deleted  or updated
 * Param:
 *  BinlogClient ptr: binlog parser context
 * Return:
 *  BinlogRow ptr: One row from binlog.
 * */
BinlogRow* fetchOne(BinlogClient *bc);
/*Free BinlogRow we just fetched
 * Param:
 *  BinlogRow ptr
 * */
void freeBinlogRow(BinlogRow *row);
/*Free libbinlog context
 * Param:
 *  BinlogClient ptr
 * */
void freeBinlogClient(BinlogClient *bc);
/*Print a Cell of a row
 * Param:
 *  Cell ptr: A Cell is a field of one row ,see logevent.h
 * */
void printCell(Cell *cell);
#ifdef __cplusplus
}
#endif
#endif
