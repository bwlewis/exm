#Linux: sharedobject
#	export LD_PRELOAD=$(pwd)/exm.so

#Mac: sharedobject
#	export DYLD_INSERT_LIBRARIES="$(PWD)/exm.so"

ifndef PREFIX
  PREFIX = /usr/local/
endif

all: lib

lib:
	$(CC) $(CFLAGS) -Wall -fopenmp -I. -fPIC -shared -c api.c
	$(CC) $(CFLAGS) -Wall -fopenmp -I. -fPIC -shared -o libexm.so api.o exm.c -ldl

clean:
	rm -f *.so *.o  test

test: lib
	$(CC) -o test test.c -ldl
	LD_PRELOAD=$(shell pwd)/libexm.so ./test

install: lib
	mkdir -p $(PREFIX)/bin $(PREFIX)/lib
	cat exm | sed -e "s%EXM_HOME=$$%EXM_HOME=${PREFIX}%" > $(PREFIX)/bin/exm
	chmod +x $(PREFIX)/bin/exm
	cp libexm.so $(PREFIX)/lib

uninstall:
	rm -f $(PREFIX)/bin/exm
	rm -f $(PREFIX)/lib/libexm.so
