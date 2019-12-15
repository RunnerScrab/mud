CC = gcc
CFLAGS = -Wall -DDEBUG
LDFLAGS = -pthread -lcheck
DEBUG_CFLAGS = -DDEBUG
DEBUG_LDFLAGS = -g -lm -pg

SRC_DIR ?= ./
TEST_DIR ?= ./

MEMOBJ = talloc.o poolalloc.o
ADTOBJ =  vector.o charvector.o heap.o
SERVEROBJ = mud.o server.o client.o threadpool.o
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

clean:
	rm -f *.o gmon.out *.core *~ mud
