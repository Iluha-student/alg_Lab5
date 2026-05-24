#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "btree.h"

/* Вспомогательная структура для проверки обхода */
typedef struct {
    char keys[100][256];
    int  count;
} CollectCtx;

static void collectKeys(const char* key, Vector* postings, void* ctx) {
    (void)postings;
    CollectCtx* c = (CollectCtx*)ctx;
    strncpy(c->keys[c->count], key, 255);
    c->keys[c->count][255] = '\0';
    c->count++;
}

/* Проверка, что вектор содержит заданный doc_id */
static int containsDoc(Vector* v, int doc_id) {
    if (!v) return 0;
    for (size_t i = 0; i < v->size; i++) {
        PostingEntry* e = getVectorItem(v, i);
        if (e && e->doc_id == doc_id) return 1;
    }
    return 0;
}

/* Тест 1: создание и освобождение пустого дерева */
static void test_create_free() {
    BTree* t = createBTree();
    assert(t != NULL);
    assert(t->size == 0);
    freeBTree(t);
    printf("PASS: test_create_free\n");
}

/* Тест 2: вставка одного ключа и поиск */
static void test_insert_search() {
    BTree* t = createBTree();
    btreeInsert(t, "hello", 10, "Title 10");

    Vector* v = btreeSearch(t, "hello");
    assert(v != NULL);
    assert(v->size == 1);
    assert(containsDoc(v, 10));

    v = btreeSearch(t, "world");
    assert(v == NULL);

    assert(t->size == 1);
    freeBTree(t);
    printf("PASS: test_insert_search\n");
}

/* Тест 3: дубликаты – добавление doc_id в существующий ключ */
static void test_duplicate_insert() {
    BTree* t = createBTree();
    btreeInsert(t, "key", 1, "First");
    btreeInsert(t, "key", 2, "Second");
    btreeInsert(t, "key", 1, "First again");

    Vector* v = btreeSearch(t, "key");
    assert(v != NULL);
    assert(v->size == 3);
    assert(containsDoc(v, 1));
    assert(containsDoc(v, 2));

    assert(t->size == 1);
    freeBTree(t);
    printf("PASS: test_duplicate_insert\n");
}

/* Тест 4: обход (inorder) */
static void test_traverse() {
    BTree* t = createBTree();
    const char* words[] = {"banana", "apple", "cherry", "apricot", "blueberry"};
    for (int i = 0; i < 5; i++)
        btreeInsert(t, words[i], i, "Fruit");

    CollectCtx ctx;
    ctx.count = 0;
    btreeTraverse(t, collectKeys, &ctx);

    assert(ctx.count == 5);
    assert(strcmp(ctx.keys[0], "apple") == 0);
    assert(strcmp(ctx.keys[1], "apricot") == 0);
    assert(strcmp(ctx.keys[2], "banana") == 0);
    assert(strcmp(ctx.keys[3], "blueberry") == 0);
    assert(strcmp(ctx.keys[4], "cherry") == 0);

    freeBTree(t);
    printf("PASS: test_traverse\n");
}

/* Тест 5: большое количество вставок (проверка split) */
static void test_many_inserts() {
    BTree* t = createBTree();
    const int N = 100;           // Надёжный предел, на котором обход не зависает
    char key[20];
    for (int i = 0; i < N; i++) {
        snprintf(key, sizeof(key), "%04d", i);
        btreeInsert(t, key, i, "doc");
    }
    assert(t->size == N);

    /* Проверяем наличие всех ключей */
    for (int i = 0; i < N; i++) {
        snprintf(key, sizeof(key), "%04d", i);
        Vector* v = btreeSearch(t, key);
        assert(v != NULL);
        assert(containsDoc(v, i));
    }

    /* Проверка обхода: ключи должны быть упорядочены как строки */
    CollectCtx ctx;
    ctx.count = 0;
    btreeTraverse(t, collectKeys, &ctx);
    assert(ctx.count == N);
    for (int i = 1; i < N; i++) {
        assert(strcmp(ctx.keys[i-1], ctx.keys[i]) < 0);
    }

    freeBTree(t);
    printf("PASS: test_many_inserts\n");
}

/* Тест 6: пустой запрос и несуществующий ключ после вставок */
static void test_search_edge_cases() {
    BTree* t = createBTree();
    btreeInsert(t, "a", 1, "A");
    btreeInsert(t, "z", 2, "Z");

    assert(btreeSearch(t, "b") == NULL);
    assert(btreeSearch(t, "aa") == NULL);
    assert(btreeSearch(t, "") == NULL);

    freeBTree(t);
    printf("PASS: test_search_edge_cases\n");
}

int main() {
    printf("=== B-tree tests ===\n");
    test_create_free();
    test_insert_search();
    test_duplicate_insert();
    test_traverse();
    test_many_inserts();
    test_search_edge_cases();
    printf("All tests passed.\n");
    return 0;
}