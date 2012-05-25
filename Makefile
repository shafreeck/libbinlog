.PHONY:lib test install clean
lib:
	cd src && make
test:
	cd test && make
install:
	cd src && make install
clean:
	cd src && make clean
