CC = gcc
FLAGS = -pthread -g

mud: mud.o talloc.o vector.o heap.o client.o
	$(CC) mud.o talloc.o vector.o heap.o client.o -o mud $(FLAGS)
mud.o: mud.c
	$(CC) -c mud.c $(FLAGS)
talloc.o: talloc.c talloc.h
	$(CC) -c talloc.c $(FLAGS)
vector.o: vector.c vector.h talloc.o
	$(CC) -c vector.c $(FLAGS)
heap.o: heap.c heap.h
	$(CC) -c heap.c $(FLAGS)

client.o: client.c client.h talloc.o
	$(CC) -c client.c $(FLAGS)

test_heap: test_heap.o talloc.o heap.o
	$(CC) test_heap.o talloc.o heap.o -o test_heap $(FLAGS)

test_heap.o: test_heap.c
	$(CC) -c test_heap.c $(FLAGS)
testvector.o: testvector.c
	$(CC) -c testvector.c $(FLAGS)

testvector: testvector.o mud
	$(CC) testvector.o talloc.o vector.o -o testvector $(FLAGS)

test_threadpool: test_threadpool.o talloc.o heap.o vector.o
	$(CC) test_threadpool.o talloc.o heap.o vector.o -o test_threadpool $(FLAGS)
test_threadpool.o: test_threadpool.c
	$(CC) -c test_threadpool.c $(FLAGS)

clean:
	rm -f mud *.o a.out *~


