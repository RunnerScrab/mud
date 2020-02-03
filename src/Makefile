CC = gcc
CCXX = g++

SQLITECC = gcc
CFLAGS = -Wall -Ofast
CXXFLAGS = -Wall -Ofast
LDLIBS = -pthread -lz -ldl -lm -Wl,-soname,libsodium.so,-rpath='./' -L. -l:libsodium.so
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
# The linking must be managed by g++
	${CCXX} -o mud ${OBJS} ${LIBS} ${LDLIBS}

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
