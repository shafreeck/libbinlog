#ifndef LIBBINLOG_BINARY_H
#define LIBBINLOG_BINARY_H
#include "vio.h"
#include <stdint.h>
#include <assert.h>

/*return 1 on success and 0 on error*/
void readBinary(vio *v,char *buf,size_t len);
char *readStr(vio *v);

int8_t readInt8(vio *v);
uint8_t readUint8(vio *v);

int16_t readInt16(vio *v);
uint16_t readUint16(vio *v);

int32_t readInt24(vio *v);
uint32_t readUint24(vio *v);

int32_t readInt32(vio *v);
uint32_t readUint32(vio *v);

int64_t readInt64(vio *v);
uint64_t readUint64(vio *v);

void writeBinary(vio *v,const char *buf,size_t len);

#endif
