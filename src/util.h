/*util functions*/
#ifndef __UTIL_H
#define __UTIL_H

#include <stdint.h>
#include "logevent.h"
#define getUint32(ev) (*(uint32_t*)(ev))
#define getUint64(ev) (*(uint64_t*)(ev))
#define getUint8(ev) (*(uint8_t*)(ev))
#define getUint16(ev) (*(uint16_t*)(ev))
#define getInt32(ev) (*(int32_t*)(ev))
#define getInt64(ev) (*(int64_t*)(ev))
#define getInt8(ev) (*(int8_t*)(ev))
#define getInt16(ev) (*(int16_t*)(ev))
#define getUint48(A)((uint64_t)(((uint32_t)((uint8_t) (A)[0])) + \
			(((uint32_t)((uint8_t) (A)[1])) << 8)   + \
			(((uint32_t)((uint8_t) (A)[2])) << 16)  + \
			(((uint32_t)((uint8_t) (A)[3])) << 24)) + \
		(((uint64_t) ((uint8_t) (A)[4])) << 32) + \
		(((uint64_t) ((uint8_t) (A)[5])) << 40))

#define getUint24(A) (uint32_t)(((uint32_t)((uint8_t) (A)[0])) +\
		(((uint32_t) ((uint8_t) (A)[1])) << 8) +\
		(((uint32_t) ((uint8_t) (A)[2])) << 16))
#define getInt24(A) (int32_t)(((int32_t)((int8_t) (A)[0])) +\
		(((int32_t) ((int8_t) (A)[1])) << 8) +\
		(((int32_t) ((int8_t) (A)[2])) << 16))

/*end util functions*/
#endif
