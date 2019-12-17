CC = gcc
CFLAGS = -Wall -DDEBUG -g
LDFLAGS = -pthread -lcheck -g
DEBUG_CFLAGS = -DDEBUG
DEBUG_LDFLAGS = -g -lm -pg

SRC_DIR ?= ./
TEST_DIR ?= ./

MEMOBJ = talloc.o poolalloc.o
ADTOBJ =  vector.o charvector.o heap.o
SERVEROBJ = mud.o server.o client.o threadpool.o iohelper.o telnet.o
OBJ = $(MEMOBJ) $(ADTOBJ) $(SERVEROBJ)

TESTSRC := $(shell find $(TEST_DIR) -regextype sed -regex ".*/test.*\.c")

MEMSRC = $(patsubst %.o,%.c,$(MEMOBJ))
ADTSRC = $(patsubst %.o,%.c,$(ADTOBJ))
SERVERSRC = $(patsubst %.o,%.c,$(SERVEROBJ))
SRC = $(MEMSRC) $(ADTSRC) $(SERVERSRC)


TESTOBJ = $(patsubst %.c,%.o,$(TESTSRC))


mud: $(OBJ)
	$(CC) $(OBJ) -o mud $(LDFLAGS)

%.o:%.c $(SRC) $(patsubst %.c,%.h,$(SRC))
	$(CC) -c $< $(CFLAGS)

test_charvector.o: test_charvector.c

test_charvector: talloc.o test_charvector.o charvector.o
	$(CC) $^ -o $@ $(LDFLAGS)

test_iohelper.o: test_iohelper.c

test_iohelper: talloc.o charvector.o iohelper.o test_iohelper.o
	$(CC) $^ -o $@ $(LDFLAGS)

clean:
	rm -f *.o gmon.out *.core *~ mud
