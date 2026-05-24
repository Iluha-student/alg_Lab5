CC     = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -g
INCLUDE = -I. -I lab3/vector

OBJ_SHARED = posting.o generic.o avl/avl.o rbtree/rbtree.o btree/btree.o \
             index/index.o index/search.o

.PHONY: all app test_avl test_rb test_btree u_tests clean

all: app

app: $(OBJ_SHARED) main.o
	$(CC) $(CFLAGS) -o app $(OBJ_SHARED) main.o

generic.o: lab3/vector/generic.c lab3/vector/generic.h
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

posting.o: posting.c posting.h
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

avl/avl.o: avl/avl.c avl/avl.h
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

rbtree/rbtree.o: rbtree/rbtree.c rbtree/RBTree.h
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

btree/btree.o: btree/btree.c btree/btree.h
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

index/index.o: index/index.c index/index.h
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

index/search.o: index/search.c index/search.h
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

main.o: main.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

test_avl: generic.o posting.o avl/avl.o avl/tests.o
	$(CC) $(CFLAGS) -o test_avl $^

test_rb: generic.o posting.o rbtree/rbtree.o rbtree/tests.o
	$(CC) $(CFLAGS) -o test_rb $^

test_btree: generic.o posting.o btree/btree.o btree/tests.o
	$(CC) $(CFLAGS) -o test_btree $^

u_tests: test_avl test_rb test_btree
	./test_avl
	./test_rb
	./test_btree

clean:
	rm -f app test_avl test_rb test_btree
	rm -f *.o avl/*.o rbtree/*.o btree/*.o index/*.o

