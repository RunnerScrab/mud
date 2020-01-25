CC = g++
SQLITECC = gcc
CFLAGS = -Wall -g
LDLIBS = -pthread -lcheck -lz -ldl -l:libsodium.so
OUTPUT_OPTION = -MMD -MP -o $@

CSOURCE := $(wildcard *.c)
CXXSOURCE := $(wildcard *.cpp)

COBJS := $(CSOURCE:.c=.o)
CXXOBJS := $(CXXSOURCE:.cpp=.o)

CDEPS := $(CSOURCE:.c=.d)
CXXDEPS := $(CXXSOURCE:.cpp=.d)


OBJS := $(COBJS) $(CXXOBJS)
DEPS := $(CDEPS) $(CXXDEPS)
LIBS = libangelscript.a libsqlite3.a

mud: ${OBJS} ${LIBS}
	${CC} -o mud ${OBJS} ${LIBS} ${LDLIBS}

sqlite: sqlite3.o

libsqlite3.a: sqlite/sqlite3.c
	${SQLITECC} -c sqlite/sqlite3.c ${CFLAGS}
	ar rcs libsqlite3.a sqlite3.o

angelscript:
	make -C angelscriptsdk/sdk/angelscript/projects/gnuc/
	cp angelscriptsdk/sdk/angelscript/include/angelscript.h .
	cp angelscriptsdk/sdk/angelscript/lib/libangelscript.a .

libsodium.so:
	(cd ./libsodium && ./autogen.sh)
	(cd ./libsodium/ && ./configure)
	make -C libsodium/
	cp libsodium/src/libsodium/.libs/libsodium.so* .

-include ${DEPS}

clean:
	rm -f *.o mud *.d *~
	cd angelscriptsdk/sdk/angelscript/projects/gnuc/ && make clean

etags:
	etags *.c *.h
