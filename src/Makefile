CC = g++
CFLAGS = -Wall -g -DDEBUG
LDLIBS = -pthread -lcheck -lz
OUTPUT_OPTION = -MMD -MP -o $@

SOURCE =  $(wildcard *.c)
OBJS = $(SOURCE:.c=.o)
DEPS = $(SOURCE:.c=.d)

mud: ${OBJS}
	${CC} -o mud ${OBJS} ${LDLIBS}

angelscript:
	make -C angelscriptsdk/angelscript/projects/gnuc/
	cp angelscriptsdk/angelscript/include/angelscript.h .
	cp angelscriptsdk/angelscript/lib/libangelscript.a .

-include ${DEPS}

clean:
	rm -f *.o mud *.d *~

etags:
	etags *.c *.h
