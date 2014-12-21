#Linux: sharedobject
#	export LD_PRELOAD=$(pwd)/exm.so

#Mac: sharedobject
#	export DYLD_INSERT_LIBRARIES="$(PWD)/exm.so"

ifndef PREFIX
  PREFIX = /usr/local/
endif

all: lib

lib:
	$(CC) -Wall -fopenmp -I. -fPIC -shared -c api.c
	$(CC) -Wall -fopenmp -I. -fPIC -shared -o libexm.so api.o exm.c -ldl

clean:
	rm -f *.so *.o  test

test:
	$(CC) -o test test.c -ldl

install:
	mkdir -p $(PREFIX)/bin $(PREFIX)/lib
	cat exm | sed -e "s%FLEXMEM_HOME=$$%FLEXMEM_HOME=${PREFIX}%" > $(PREFIX)/bin/exm
	chmod +x $(PREFIX)/bin/exm
	cp libexm.so $(PREFIX)/lib

uninstall:
	rm -f $(PREFIX)/bin/exm
	rm -f $(PREFIX)/lib/libexm.so
