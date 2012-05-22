#ifndef __FILEDRIBER_H
#define __FILEDRIBER_H
#include <sys/types.h>
#include "libbinlog.h"

typedef struct filedriver_st{
	int fd;
	char *errstr;
	uint64_t position;
	int index;
	char buf[BL_EVENT_BUF_SIZE];
	char *newbuf;
}FileDriver;

int openFile(const char* file);
void closeFile(int fd);

uint8_t *getEventFromFile(void *args);
void freeEventFromFile(void *args);

#endif
