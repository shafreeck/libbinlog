all=test_vio test_binary test_proto
obj=../vio.o ../binary.o ../anet.o ../protocol.o ../sha1.o

test_vio_obj=test_vio.o
test_binary_obj=test_binary.o
test_proto_obj=test_proto.o

all:$(all)
test_vio:$(obj) $(test_vio_obj)
	gcc -o test_vio $(obj) $(test_vio_obj)
test_binary:$(obj) $(test_binary_obj)
	gcc -o test_binary $(obj) $(test_binary_obj)
test_proto:$(test_proto_obj) $(obj)
clean:
	rm -f $(all) $(obj)
