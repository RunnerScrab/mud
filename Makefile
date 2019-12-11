CC = gcc
CFLAGS = -DINTEL -DCACHE_LINE_SIZE=`getconf LEVEL1_DCACHE_LINESIZE` -std=c99 -DNDEBUG
LDFLAGS = -lpthread -lm -lrt

mud: mud.o talloc.o vector.o heap.o client.o threadpool.o poolalloc.o
	$(CC) mud.o threadpool.o poolalloc.o talloc.o vector.o heap.o client.o -o mud $(FLAGS) $(LDFLAGS)
mud.o: mud.c
	$(CC) -c mud.c $(FLAGS)
talloc.o: talloc.c talloc.h
	$(CC) -c talloc.c $(FLAGS)
vector.o: vector.c vector.h talloc.o
	$(CC) -c vector.c $(FLAGS)
heap.o: heap.c heap.h
	$(CC) -c heap.c $(FLAGS)

threadpool.o: threadpool.c threadpool.h
	$(CC) -c threadpool.c $(FLAGS)

poolalloc.o: poolalloc.c poolalloc.h
	$(CC) -c poolalloc.c $(FLAGS)

client.o: client.c client.h talloc.o
	$(CC) -c client.c $(FLAGS)


prioq: prioq/gc/gc.o prioq/gc/ptst.o prioq/prioq.o prioq/common.o
	ar rcs lprioq.a prioq/gc/gc.o prioq/gc/ptst.o prioq/prioq.o prioq/common.o
gc.o: prioq/gc/gc.c prioq/gc/gc.h
	$(CC) -c prioq/gc.c $(CFLAGS)
ptst.o: prioq/gc/ptst.c prioq/gc/ptst.h
	$(CC) -c prioq/gc/ptst.c $(CFLAGS)
prioq.o: prioq/prioq.c prioq/prioq.h
	$(CC) -c prioq/prioq.c $(CFLAGS)
common.o: prioq/common.c prioq/common.h
	$(CC) -c prioq/common.c $(CFLAGS)

test: test_heap testvector test_poolallocator test_threadpool test_lprioq

test_lprioq: test_lprioq.o talloc.o prioq/gc/gc.o prioq/gc/ptst.o prioq/prioq.o prioq/common.o
	$(CC) test_lprioq.o prioq/gc/gc.o prioq/gc/ptst.o prioq/prioq.o prioq/common.o talloc.o -o test_lprioq $(CFLAGS) $(LDFLAGS)

test_lprioq.o: test_lprioq.c
	$(CC) -c test_lprioq.c $(CFLAGS)

test_heap: test_heap.o talloc.o heap.o
	$(CC) test_heap.o talloc.o heap.o -o test_heap $(FLAGS) $(LDFLAGS)

test_heap.o: test_heap.c
	$(CC) -c test_heap.c $(FLAGS)
testvector.o: testvector.c
	$(CC) -c testvector.c $(FLAGS)

testvector: testvector.o mud
	$(CC) testvector.o talloc.o vector.o -o testvector $(FLAGS) $(LDFLAGS)

test_poolallocator: test_poolallocator.o talloc.o poolalloc.o
	$(CC) test_poolallocator.o talloc.o poolalloc.o -o test_poolallocator $(FLAGS) $(LDFLAGS)

test_poolallocator.o: test_poolallocator.c
	$(CC) -c test_poolallocator.c $(FLAGS)

test_threadpool: test_threadpool.o poolalloc.o talloc.o heap.o vector.o threadpool.o
	$(CC) test_threadpool.o poolalloc.o threadpool.o talloc.o heap.o vector.o -o test_threadpool $(FLAGS) $(LDFLAGS)

test_threadpool.o: test_threadpool.c
	$(CC) -c test_threadpool.c $(FLAGS)

clean:
	rm -f mud *.o a.out *~ #* prioq/*.o prioq/gc/*.o
