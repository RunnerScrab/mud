CC = gcc
CFLAGS = -Wall -DDEBUG
LDFLAGS = -pthread

SRC_DIRS ?= ./

MEMOBJ = talloc.o poolalloc.o
ADTOBJ =  vector.o charvector.o heap.o
SERVEROBJ = mud.o server.o client.o threadpool.o
OBJ = $(MEMOBJ) $(ADTOBJ) $(SERVEROBJ)

MEMSRC = $(patsubst %.o,%.c,$(MEMOBJ))
ADTSRC = $(patsubst %.o,%.c,$(ADTOBJ))
SERVERSRC = $(patsubst %.o,%.c,$(SERVEROBJ))

mud: $(OBJ)
	$(CC) $(OBJ) -o mud $(LDFLAGS)

%.o: %.c $(MEMSRC) $(patsubst %.c,%.h,$(MEMSRC))
	$(CC) -c -o $@ $< $(CFLAGS)

%.o: %.c $(ADTSRC) $(patsubst %.c,%.h,$(ADTSRC))
	$(CC) -c -o $@ $< $(CFLAGS)

%.o: %.c $(SERVERSRC) $(patsubst %.c,%.h,$(SERVERSRC))
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f *.o gmon.out *.core *~ mud
