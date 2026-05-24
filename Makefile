CC     = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -g
INCLUDE = -I. -I lab3/vector

OBJ_SHARED = posting.o generic.o avl/avl.o rbtree/rbtree.o btree/btree.o \
             index/index.o index/search.o

.PHONY: all app u_tests test clean

all: app u_tests

app: $(OBJ_SHARED) main.o
	$(CC) $(CFLAGS) -o app $(OBJ_SHARED) main.o

generic.o: lab3/vector/generic.c lab3/vector/generic.h
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

posting.o: posting.c posting.h
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

avl/avl.o: avl/avl.c avl/avl.h
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

rbtree/rbtree.o: rbtree/rbtree.c rbtree/rbtree.h
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

test: app
	@echo "=== E2E: preprocessing ==="
	mkdir -p data/test
	python3 preprocess.py \
		--input  data/test/Questions.csv \
		--output data/test/docs.jsonl
	@echo "=== E2E: indexing ==="
	./app index --type=avl   --data=data/test/docs.jsonl --index=data/test/idx_avl.txt
	./app index --type=rb    --data=data/test/docs.jsonl --index=data/test/idx_rb.txt
	./app index --type=btree --data=data/test/docs.jsonl --index=data/test/idx_btree.txt
	@echo "=== E2E: searching ==="
	./app search --type=avl   --index=data/test/idx_avl.txt   --json "python list"
	./app search --type=rb    --index=data/test/idx_rb.txt    --json "python list"
	./app search --type=btree --index=data/test/idx_btree.txt --json "python list"
	@echo "=== E2E OK ==="

clean:
	rm -f app test_avl test_rb test_btree
	rm -f *.o avl/*.o rbtree/*.o btree/*.o index/*.o
	rm -f data/index_*.txt data/test/docs.jsonl data/test/idx_*.txt