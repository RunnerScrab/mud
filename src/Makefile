CC = g++
CFLAGS = -Wall -g
LDLIBS = -pthread -lcheck -lz
OUTPUT_OPTION = -MMD -MP -o $@

SOURCE = $(wildcard *.c)
OBJS = $(SOURCE:.c=.o)
DEPS = $(SOURCE:.c=.d)
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
