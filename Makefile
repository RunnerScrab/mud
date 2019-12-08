CC = gcc
FLAGS = -g

mud: mud.o talloc.o sockarray.o
	$(CC) mud.o talloc.o sockarray.o -o mud $(FLAGS)
mud.o: mud.c
	$(CC) -c mud.c $(FLAGS)
talloc.o: talloc.c talloc.h
	$(CC) -c talloc.c $(FLAGS)
sockarray.o: sockarray.c sockarray.h talloc.o
	$(CC) -c sockarray.c $(FLAGS)

clean:
	rm -f mud *.o a.out *~


