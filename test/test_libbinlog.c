#include <libbinlog/libbinlog.h>
#include <assert.h>
int main(int argc, const char *argv[]) {
	int i;
	BinlogClient  *bc = connectDataSource("mysql://root@10.210.210.146:3306/mysql-bin.000001",4,0,0);	
	if(bc->err){
		fprintf(stderr,"%s\n",bc->errstr);
		exit(1);
	}
	BinlogRow *row = fetchOne(bc);
	assert(row);
	for(i = 0; i < row->nfields; ++i){
		printCell(&(row->row[i]));
	}
	freeBinlogRow(row);

	freeBinlogClient(bc);
	return 0;
}

