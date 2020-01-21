CC = g++
CFLAGS = -Wall -g
LDLIBS = -pthread -lcheck -lz
OUTPUT_OPTION = -MMD -MP -o $@

CSOURCE := $(wildcard *.c)
CXXSOURCE := $(wildcard *.cpp)

COBJS := $(CSOURCE:.c=.o)
CXXOBJS := $(CXXSOURCE:.cpp=.o)

CDEPS := $(CSOURCE:.c=.d)
CXXDEPS := $(CXXSOURCE:.cpp=.d)


OBJS := $(COBJS) $(CXXOBJS)
DEPS := $(CDEPS) $(CXXDEPS)
LIBS = libangelscript.a

mud: ${OBJS}
	${CC} -o mud ${OBJS} ${LIBS} ${LDLIBS}

angelscript:
	make -C angelscriptsdk/sdk/angelscript/projects/gnuc/
	cp angelscriptsdk/sdk/angelscript/include/angelscript.h .
	cp angelscriptsdk/sdk/angelscript/lib/libangelscript.a .

-include ${DEPS}

clean:
	rm -f *.o mud *.d *~
	cd angelscriptsdk/sdk/angelscript/projects/gnuc/ && make clean

etags:
	etags *.c *.h
