CC = gcc
CFLAGS = -Wall -DDEBUG -g
LDLIBS = -pthread -lcheck
OUTPUT_OPTION = -MMD -MP -o $@

SOURCE =  $(wildcard *.c)
OBJS = $(SOURCE:.c=.o)
DEPS = $(SOURCE:.c=.d)
TARGS = $(patsubst %.c,%, ${SOURCE})


-include ${DEPS}

#clean:
#	rm -f *.o ${TARGS} *~ *.d

etags:
	etags *.c *.h
