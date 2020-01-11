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
	make -C angelscriptsdk/angelscript/projects/gnuc/
	cp angelscriptsdk/angelscript/include/angelscript.h .
	cp angelscriptsdk/angelscript/lib/libangelscript.a .

-include ${DEPS}

clean:
	rm -f *.o mud *.d *~
	cd angelscriptsdk/angelscript/projects/gnuc/ && make clean

etags:
	etags *.c *.h
