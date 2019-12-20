CC = gcc
CFLAGS = -Wall -DDEBUG -g
LDLIBS = -pthread -lcheck -lz
OUTPUT_OPTION = -MMD -MP -o $@

SOURCE =  $(wildcard *.c)
OBJS = $(SOURCE:.c=.o)
DEPS = $(SOURCE:.c=.d)

mud: ${OBJS}
	${CC} -o mud ${OBJS} ${LDLIBS}

-include ${DEPS}

clean:
	rm -f *.o mud

etags:
	etags *.c *.h
