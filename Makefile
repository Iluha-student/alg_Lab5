CC       = gcc
CFLAGS   = -Wall -Wextra -std=c11 -O2 -g -I. -Ilab3/vector
LDFLAGS  =

# Путь к исходникам вектора
VECTOR_SRCDIR = lab3/vector
VECTOR_OBJ    = generic.o

COMMON_OBJS = $(VECTOR_OBJ) posting.o
TREE_OBJS   = avl/avl.o rbtree/rbtree.o btree/btree.o
INDEX_OBJS  = index/index.o index/search.o
APP_OBJS    = $(COMMON_OBJS) $(TREE_OBJS) $(INDEX_OBJS) main.o

# Объекты, нужные для bench (все, кроме main.o)
BENCH_OBJS  = $(COMMON_OBJS) $(TREE_OBJS) $(INDEX_OBJS)
CFLAGS   = -Wall -Wextra -std=c11 -O2 -g -I.
LDFLAGS  =

# Общие объектные файлы (вектор, posting list)
COMMON_OBJS = generic.o posting.o

# Объектные файлы деревьев (используются в основном приложении)
TREE_OBJS = avl/avl.o rbtree/rbtree.o btree/btree.o

# Объектные файлы индекса и поиска (для приложения)
INDEX_OBJS = index/index.o index/search.o

# Все объекты для приложения
APP_OBJS = $(COMMON_OBJS) $(TREE_OBJS) $(INDEX_OBJS) main.o

# --------------------- Цели по умолчанию ---------------------
.PHONY: all app u_tests test clean

all: app u_tests

# Правило для компиляции generic.o из поддиректории
$(VECTOR_OBJ): $(VECTOR_SRCDIR)/generic.c $(VECTOR_SRCDIR)/generic.h
	$(CC) $(CFLAGS) -c -o $@ $<

# Общее правило для остальных .c файлов
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

app: $(APP_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test_avl: $(COMMON_OBJS) avl/avl.o avl/tests.o
	$(CC) $(CFLAGS) -o $@ $^

test_rb: $(COMMON_OBJS) rbtree/rbtree.o rbtree/tests.o
	$(CC) $(CFLAGS) -o $@ $^

test_btree: $(COMMON_OBJS) btree/btree.o btree/tests.o
	$(CC) $(CFLAGS) -o $@ $^
# --------------------- Приложение ---------------------
app: $(APP_OBJS)
  $(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# --------------------- Тесты ---------------------
test_avl: $(COMMON_OBJS) avl/avl.o avl/tests.o
  $(CC) $(CFLAGS) -o $@ $^

test_rb: $(COMMON_OBJS) rbtree/rbtree.o rbtree/tests.o
  $(CC) $(CFLAGS) -o $@ $^

test_btree: $(COMMON_OBJS) btree/btree.o btree/tests.o
  $(CC) $(CFLAGS) -o $@ $^

# Запуск всех модульных тестов
u_tests: test_avl test_rb test_btree
  ./test_avl
  ./test_rb
  ./test_btree

bench: bench.o $(BENCH_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lm -lpsapi

# Явное правило для bench.o, чтобы избежать неоднозначности
bench.o: bench.c
	$(CC) $(CFLAGS) -c $< -o $@

# E2E тест с ограничением количества документов (1000)
test: app
	@echo "=== E2E: preprocessing (limit 1000) ==="
	mkdir -p data/test
	python3 preprocess.py \
		--input  data/Questions.csv \
		--output data/test/docs.jsonl \
		--limit 1000
	@echo "=== E2E: indexing ==="
	./app index --type=avl   --data=data/test/docs.jsonl --index=data/test/idx_avl.txt
	./app index --type=rb    --data=data/test/docs.jsonl --index=data/test/idx_rb.txt
	./app index --type=btree --data=data/test/docs.jsonl --index=data/test/idx_btree.txt
	@echo "=== E2E: searching ==="
	./app search --type=avl   --index=data/test/idx_avl.txt   --json "python list"
	./app search --type=rb    --index=data/test/idx_rb.txt    --json "python list"
	./app search --type=btree --index=data/test/idx_btree.txt --json "python list"
	@echo "=== E2E OK ==="

# Зависимости (чтобы перекомпилировалось при изменении заголовков)
$(VECTOR_OBJ):   lab3/vector/generic.h
posting.o:       posting.h $(VECTOR_SRCDIR)/generic.h
avl/avl.o:       avl/avl.h posting.h $(VECTOR_SRCDIR)/generic.h
rbtree/rbtree.o: rbtree/rbtree.h posting.h $(VECTOR_SRCDIR)/generic.h
btree/btree.o:   btree/btree.h posting.h $(VECTOR_SRCDIR)/generic.h
index/index.o:   index/index.h posting.h $(VECTOR_SRCDIR)/generic.h
index/search.o:  index/search.h posting.h $(VECTOR_SRCDIR)/generic.h
main.o:          index/index.h

avl/tests.o:     avl/avl.h posting.h $(VECTOR_SRCDIR)/generic.h
rbtree/tests.o:  rbtree/rbtree.h posting.h $(VECTOR_SRCDIR)/generic.h
btree/tests.o:   btree/btree.h posting.h $(VECTOR_SRCDIR)/generic.h
# --------------------- End-to-end тесты ---------------------
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

# --------------------- Правила компиляции ---------------------
# Общее правило для всех .c файлов (включая поддиректории)
%.o: %.c
  $(CC) $(CFLAGS) -c -o $@ $<

# Явные правила для объектных файлов из поддиректорий (необязательно, но для ясности)
avl/avl.o: avl/avl.c avl/avl.h posting.h generic.h
rbtree/rbtree.o: rbtree/rbtree.c rbtree/rbtree.h posting.h generic.h
btree/btree.o: btree/btree.c btree/btree.h posting.h generic.h
index/index.o: index/index.c index/index.h posting.h generic.h
index/search.o: index/search.c index/search.h posting.h generic.h
main.o: main.c index/index.h

# Тестовые объекты зависят от соответствующих заголовков
avl/tests.o: avl/tests.c avl/avl.h posting.h generic.h
rbtree/tests.o: rbtree/tests.c rbtree/rbtree.h posting.h generic.h
btree/tests.o: btree/tests.c btree/btree.h posting.h generic.h

# --------------------- Очистка ---------------------
clean:
	rm -f app test_avl test_rb test_btree bench
	rm -f *.o avl/*.o rbtree/*.o btree/*.o index/*.o
	rm -f data/index_*.txt data/test/docs.jsonl data/test/idx_*.txt
  rm -f app test_avl test_rb test_btree
  rm -f *.o avl/*.o rbtree/*.o btree/*.o index/*.o
  rm -f data/index_*.txt data/test/docs.jsonl data/test/idx_*.txt
