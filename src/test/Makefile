CC = gcc
CFLAGS = -Wall -DDEBUG -g
LDLIBS = -pthread -lcheck
OUTPUT_OPTION = -MMD -MP -o $@

SOURCE =  $(wildcard *.c)
OBJS = $(SOURCE:.c=.o)
DEPS = $(SOURCE:.c=.d)
TARGS = $(patsubst %.c,%, ${SOURCE})

-include ${DEPS}

test: test_charvector test_poolallocator test_iohelper

test_charvector: test_charvector.o ../talloc.o ../charvector.o
test_poolallocator: test_poolallocator.o ../talloc.o ../poolalloc.o
test_iohelper: test_iohelper.o ../talloc.o ../iohelper.o ../charvector.o
#clean:
#	rm -f *.o ${TARGS} *~ *.d

etags:
	etags *.c *.h
