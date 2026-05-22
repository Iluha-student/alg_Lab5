#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "avl.h"

// ------------------------------------------------------------------
// Вспомогательная функция проверки posting list'а
static int checkPostings(Vector *list, int expected_ids[], int count) {
    if (!list) return count == 0;
    if ((int)list->size != count) return 0;
    for (size_t i = 0; i < list->size; i++) {
        PostingEntry *e = (PostingEntry *)getVectorItem(list, i);
        if (!e) return 0;
        if (e->doc_id != expected_ids[i]) return 0;
    }
    return 1;
}

// ------------------------------------------------------------------
static void test_create_free(void) {
    AVLTree *tree = createAVLTree();
    assert(tree != NULL);
    assert(tree->root == NULL);
    assert(tree->size == 0);
    freeAVLTree(tree);
}

static void test_insert_search_single(void) {
    AVLTree *tree = createAVLTree();
    avlInsert(tree, "hello", 1, "Hello World");
    assert(tree->size == 1);

    Vector *list = avlSearch(tree, "hello");
    assert(list != NULL);
    assert(list->size == 1);
    PostingEntry *e = (PostingEntry *)getVectorItem(list, 0);
    assert(e != NULL);
    assert(e->doc_id == 1);
    assert(strcmp(e->title, "Hello World") == 0);

    assert(avlSearch(tree, "world") == NULL);
    freeAVLTree(tree);
}

static void test_insert_duplicate(void) {
    AVLTree *tree = createAVLTree();
    avlInsert(tree, "key", 10, "Title A");
    avlInsert(tree, "key", 20, "Title B");
    assert(tree->size == 1);   // размер не увеличивается

    Vector *list = avlSearch(tree, "key");
    int expected[] = {10, 20};
    assert(checkPostings(list, expected, 2));
    freeAVLTree(tree);
}

static void test_multiple_keys(void) {
    AVLTree *tree = createAVLTree();
    avlInsert(tree, "zebra", 1, "Zebra");
    avlInsert(tree, "apple", 2, "Apple");
    avlInsert(tree, "mango", 3, "Mango");
    assert(tree->size == 3);

    Vector *v = avlSearch(tree, "apple");
    assert(v != NULL && v->size == 1);
    PostingEntry *e = (PostingEntry *)getVectorItem(v, 0);
    assert(e != NULL && e->doc_id == 2);

    v = avlSearch(tree, "zebra");
    e = (PostingEntry *)getVectorItem(v, 0);
    assert(e != NULL && e->doc_id == 1);

    freeAVLTree(tree);
}

// ------------------------------------------------------------------
// Структура для сбора данных при обходе
typedef struct {
    const char **keys;
    int *sizes;
    int idx;
} TraverseData;

static void collect_traverse(const char *key, Vector *postings, void *ctx) {
    TraverseData *data = (TraverseData *)ctx;
    data->keys[data->idx] = key;
    data->sizes[data->idx] = (int)postings->size;
    data->idx++;
}

static void test_traverse(void) {
    AVLTree *tree = createAVLTree();
    avlInsert(tree, "b", 1, "B");
    avlInsert(tree, "a", 2, "A");
    avlInsert(tree, "c", 3, "C");

    const char *expected_keys[] = {"a", "b", "c"};
    int expected_sizes[] = {1, 1, 1};
    const char *collected_keys[3];
    int collected_sizes[3];

    TraverseData data = {collected_keys, collected_sizes, 0};
    avlTraverse(tree, collect_traverse, &data);

    assert(data.idx == 3);
    for (int i = 0; i < 3; i++) {
        assert(strcmp(collected_keys[i], expected_keys[i]) == 0);
        assert(collected_sizes[i] == expected_sizes[i]);
    }
    freeAVLTree(tree);
}

static void test_balance(void) {
    AVLTree *tree = createAVLTree();
    // вставляем 100 элементов в отсортированном порядке — проверим балансировку
    for (int i = 0; i < 100; i++) {
        char key[20];
        sprintf(key, "key%d", i);
        avlInsert(tree, key, i, "Title");
    }
    assert(tree->size == 100);

    for (int i = 0; i < 100; i++) {
        char key[20];
        sprintf(key, "key%d", i);
        Vector *list = avlSearch(tree, key);
        assert(list != NULL && list->size == 1);
        PostingEntry *e = (PostingEntry *)getVectorItem(list, 0);
        assert(e != NULL && e->doc_id == i);
    }
    freeAVLTree(tree);
}

// ------------------------------------------------------------------
int main(void) {
    test_create_free();
    test_insert_search_single();
    test_insert_duplicate();
    test_multiple_keys();
    test_traverse();
    test_balance();
    printf("All AVL tests passed!\n");
    return 0;
}
