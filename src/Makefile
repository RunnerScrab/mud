CC = gcc
CCXX = g++

SQLITECC = ${CC}
OPTFLAG = -Og
DEBUGFLAGS = -g -DDEBUG
CFLAGS = -Wall ${OPTFLAG} ${DEBUGFLAGS}
CXXFLAGS = -Wall ${OPTFLAG} ${DEBUGFLAGS}
LDLIBS = ${OPTFLAG} -pthread -ldl -lm -Wl,-soname,libsodium.so,-rpath='./' -L. -l:libsodium.so ${DEBUGFLAGS}
OUTPUT_OPTION = -MMD -MP -o $@

CSOURCE := $(wildcard *.c)
CXXSOURCE := $(wildcard *.cpp)
CXXSOURCE += $(wildcard as_addons/*.cpp)

COBJS := $(CSOURCE:.c=.o)
CXXOBJS := $(CXXSOURCE:.cpp=.o)

CDEPS := $(CSOURCE:.c=.d)
CXXDEPS := $(CXXSOURCE:.cpp=.d)

OBJS := $(COBJS) $(CXXOBJS)
DEPS := $(CDEPS) $(CXXDEPS)
LIBS = libangelscript.a libsqlite3.a libsodium.so libgmp.a libz.a

all: mud
mud: ${OBJS} ${LIBS}
# The linking must be managed by g++
	${CCXX} -o mud ${OBJS} ${LIBS} ${LDLIBS}

sqlite: sqlite3.o

libsqlite3.a: sqlite/sqlite3.c
	${SQLITECC} -c sqlite/sqlite3.c ${CFLAGS}
	ar rcs libsqlite3.a sqlite3.o

libangelscript.a:
	make -C angelscriptsdk/sdk/angelscript/projects/gnuc/
	cp angelscriptsdk/sdk/angelscript/include/angelscript.h .
	cp angelscriptsdk/sdk/angelscript/lib/libangelscript.a .

libgmp.a:
	(cd ./gmp && ./configure)
	make -C gmp/
	cp gmp/.libs/libgmp.a .

libsodium.so:
	(cd ./libsodium && ./autogen.sh)
	(cd ./libsodium/ && ./configure)
	make -C libsodium/
	cp libsodium/src/libsodium/.libs/libsodium.so* .

zlib.a:
	(cd ./zlib && ./configure)
	make -C zlib/
	cp zlib/libz.a .

-include ${DEPS}

clean:
	rm -f *.o mud *.d *~ ${LIBS}
	cd angelscriptsdk/sdk/angelscript/projects/gnuc/ && make clean

etags:
	etags *.cpp *.c *.h
