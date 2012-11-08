PREFIX?=/usr/local
CFLAGS=-g -Wall -fPIC
LDFLAGS=-lm

#target to generate
all=bldump libbinlog.so libbinlog.a
obj= logevent.o filedriver.o decimal.o libbinlog.o netdriver.o mysql.o \
	protocol/protocol.o protocol/binary.o protocol/sha1.o protocol/vio.o anet.o

.PHONY:all install clean
all:$(all)
bldump:bldump.o $(obj)
libbinlog.so:$(obj)
	gcc -o libbinlog.so -shared $(obj)
libbinlog.a:
	ar r libbinlog.a $(obj)

HEADERS= constant.h logevent.h libbinlog.h datasource.h mysql.h
install:
	make all
	mkdir -p $(PREFIX)/include/libbinlog/
	cp -f $(HEADERS) $(PREFIX)/include/libbinlog/
	mkdir -p $(PREFIX)/lib
	cp -f libbinlog.so libbinlog.a $(PREFIX)/lib/

clean:
	rm -f $(all) $(obj) *.o
