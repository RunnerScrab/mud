CC = gcc
FLAGS = -g

mud: mud.o talloc.o vector.o
	$(CC) mud.o talloc.o vector.o -o mud $(FLAGS)
mud.o: mud.c
	$(CC) -c mud.c $(FLAGS)
talloc.o: talloc.c talloc.h
	$(CC) -c talloc.c $(FLAGS)
vector.o: vector.c vector.h talloc.o
	$(CC) -c vector.c $(FLAGS)



testvector.o: testvector.c
	$(CC) -c testvector.c $(FLAGS)

testvector: testvector.o mud
	$(CC) testvector.o talloc.o vector.o -o testvector $(FLAGS)


clean:
	rm -f mud *.o a.out *~


