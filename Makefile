CC = gcc
FLAGS = -Wall -O3 -pthread -DDEBUG
TESTFLAGS = -pg -g

mud: mud.o server.o talloc.o vector.o heap.o client.o threadpool.o poolalloc.o
	$(CC) mud.o server.o threadpool.o poolalloc.o talloc.o vector.o heap.o client.o -o mud $(FLAGS) $(LDFLAGS)
mud.o: mud.c
	$(CC) -c mud.c $(FLAGS)
server.o: server.c
	$(CC) -c server.c $(FLAGS)

clientvec.o: clientvector.c clientvector.h
	$(CC) -c clientvector.c $(FLAGS)

talloc.o: talloc.c talloc.h
	$(CC) -c talloc.c $(FLAGS)
vector.o: vector.c vector.h talloc.o
	$(CC) -c vector.c $(FLAGS) $(TESTFLAGS)
heap.o: heap.c heap.h
	$(CC) -c heap.c $(FLAGS) $(TESTFLAGS)

threadpool.o: threadpool.c threadpool.h
	$(CC) -c threadpool.c $(FLAGS) $(TESTFLAGS)

poolalloc.o: poolalloc.c poolalloc.h
	$(CC) -c poolalloc.c $(FLAGS) $(TESTFLAGS)

client.o: client.c client.h talloc.o
	$(CC) -c client.c $(FLAGS)

test: test_heap testvector test_poolallocator test_threadpool

test_heap: test_heap.o talloc.o heap.o
	$(CC) test_heap.o talloc.o heap.o -o test_heap $(FLAGS) $(TESTFLAGS)

test_heap.o: test_heap.c
	$(CC) -c test_heap.c $(FLAGS) $(TESTFLAGS)

test_clientvec: talloc.o
	$(CC) talloc.o test_clientvec.c -o test_clientvec $(FLAGS) $(TESTFLAGS)

testvector.o: testvector.c
	$(CC) -c testvector.c $(FLAGS) $(TESTFLAGS)
testvector: testvector.o mud
	$(CC) testvector.o talloc.o vector.o -o testvector $(FLAGS) $(TESTFLAGS)

test_poolallocator: test_poolallocator.o talloc.o poolalloc.o
	$(CC) test_poolallocator.o talloc.o poolalloc.o -o test_poolallocator $(FLAGS) $(TESTFLAGS)
test_poolallocator.o: test_poolallocator.c
	$(CC) -c test_poolallocator.c $(FLAGS) $(TESTFLAGS)

test_threadpool: test_threadpool.o poolalloc.o talloc.o heap.o vector.o threadpool.o
	$(CC) test_threadpool.o poolalloc.o threadpool.o talloc.o heap.o vector.o -o test_threadpool $(FLAGS) $(TESTFLAGS)

test_threadpool.o: test_threadpool.c
	$(CC) -c test_threadpool.c $(FLAGS) $(TESTFLAGS)

clean:
	rm -f mud *.o a.out *~ #*
