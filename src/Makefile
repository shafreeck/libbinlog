CFLAGS=-g -Wall
all=bldump
obj= logevent.o  adlist.o filedriver.o decimal.o libbinlog.o netdriver.o mysql.o \
	protocol/protocol.o protocol/binary.o protocol/sha1.o protocol/vio.o anet.o
all:$(all)
bldump:bldump.o $(obj)

clean:
	rm $(all) $(obj)
