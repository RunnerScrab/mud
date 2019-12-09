CC = gcc
FLAGS = -g

mud: mud.o talloc.o vector.o heap.o
	$(CC) mud.o talloc.o vector.o heap.o -o mud $(FLAGS)
mud.o: mud.c
	$(CC) -c mud.c $(FLAGS)
talloc.o: talloc.c talloc.h
	$(CC) -c talloc.c $(FLAGS)
vector.o: vector.c vector.h talloc.o
	$(CC) -c vector.c $(FLAGS)
heap.o: heap.c heap.h
	$(CC) -c heap.c $(FLAGS)

test_heap: test_heap.o talloc.o heap.o
	$(CC) test_heap.o talloc.o heap.o -o test_heap $(FLAGS)

test_heap.o: test_heap.c
	$(CC) -c test_heap.c $(FLAGS)
testvector.o: testvector.c
	$(CC) -c testvector.c $(FLAGS)

testvector: testvector.o mud
	$(CC) testvector.o talloc.o vector.o -o testvector $(FLAGS)


clean:
	rm -f mud *.o a.out *~


