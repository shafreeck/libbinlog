all=scheme test_libbinlog
all:$(all)
scheme:scheme.o
test_libbinlog:test_libbinlog.o
	gcc -o  test_libbinlog test_libbinlog.c -lbinlog

clean:
	rm $(all) *.o

