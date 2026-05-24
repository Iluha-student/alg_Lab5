// Тут ваши тесты для красно-черного дерева. 
// Можете использовать любой фреймворк для тестирования, который вам нравится, или просто писать функции, которые вызываются из main.c. 
// Главное — убедиться, что ваши тесты покрывают все важные случаи (вставка, удаление, поиск, балансировка и т.д.).
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "RBTree.h"

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

static void test_create_free(void) {
    RBTree *tree = createRBTree();
    assert(tree != NULL);
    assert(tree->root == tree->nil);
    assert(tree->size == 0);
    freeRBTree(tree);
}

static void test_insert_search_single(void) {
    RBTree *tree = createRBTree();
    rbInsert(tree, "hello", 1, "Hello World");
    assert(tree->size == 1);

    Vector *list = rbSearch(tree, "hello");
    assert(list != NULL);
    assert(list->size == 1);
    PostingEntry *e = (PostingEntry *)getVectorItem(list, 0);
    assert(e != NULL);
    assert(e->doc_id == 1);
    assert(strcmp(e->title, "Hello World") == 0);

    assert(rbSearch(tree, "world") == NULL);
    freeRBTree(tree);
}

static void test_insert_duplicate(void) {
    RBTree *tree = createRBTree();
    rbInsert(tree, "key", 10, "Title A");
    rbInsert(tree, "key", 20, "Title B");
    assert(tree->size == 1);

    Vector *list = rbSearch(tree, "key");
    int expected[] = {10, 20};
    assert(checkPostings(list, expected, 2));
    freeRBTree(tree);
}

static void test_multiple_keys(void) {
    RBTree *tree = createRBTree();
    rbInsert(tree, "zebra", 1, "Zebra");
    rbInsert(tree, "apple", 2, "Apple");
    rbInsert(tree, "mango", 3, "Mango");
    assert(tree->size == 3);

    Vector *v = rbSearch(tree, "apple");
    assert(v != NULL && v->size == 1);
    PostingEntry *e = (PostingEntry *)getVectorItem(v, 0);
    assert(e != NULL && e->doc_id == 2);

    v = rbSearch(tree, "zebra");
    e = (PostingEntry *)getVectorItem(v, 0);
    assert(e != NULL && e->doc_id == 1);

    freeRBTree(tree);
}

static int isRed(RBNode *node) {
    return node != NULL && node->color == RB_RED;
}

static int checkRBProperties(RBTree *tree, RBNode *node, int *black_count) {
    if (node == tree->nil) {
        *black_count = 1;
        return 1;
    }
    
    if (node == tree->root && node->color != RB_BLACK) {
        return 0;
    }
    
    if (isRed(node) && isRed(node->parent)) {
        return 0;
    }
    
    int left_black = 0, right_black = 0;
    if (!checkRBProperties(tree, node->left, &left_black)) return 0;
    if (!checkRBProperties(tree, node->right, &right_black)) return 0;
    
    if (left_black != right_black) return 0;
    
    *black_count = left_black + (node->color == RB_BLACK ? 1 : 0);
    return 1;
}

static void test_rb_properties(void) {
    RBTree *tree = createRBTree();
    
    for (int i = 0; i < 100; i++) {
        char key[20];
        sprintf(key, "key%d", i);
        rbInsert(tree, key, i, "Title");
    }
    
    int black_count = 0;
    assert(checkRBProperties(tree, tree->root, &black_count) == 1);
    assert(tree->root->color == RB_BLACK);
    
    freeRBTree(tree);
}

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
    RBTree *tree = createRBTree();
    rbInsert(tree, "b", 1, "B");
    rbInsert(tree, "a", 2, "A");
    rbInsert(tree, "c", 3, "C");

    const char *expected_keys[] = {"a", "b", "c"};
    int expected_sizes[] = {1, 1, 1};
    const char *collected_keys[3];
    int collected_sizes[3];

    TraverseData data = {collected_keys, collected_sizes, 0};
    rbTraverse(tree, collect_traverse, &data);

    assert(data.idx == 3);
    for (int i = 0; i < 3; i++) {
        assert(strcmp(collected_keys[i], expected_keys[i]) == 0);
        assert(collected_sizes[i] == expected_sizes[i]);
    }
    freeRBTree(tree);
}

static void test_large_insert(void) {
    RBTree *tree = createRBTree();
    for (int i = 0; i < 1000; i++) {
        char key[20];
        sprintf(key, "key%d", i);
        rbInsert(tree, key, i, "Title");
    }
    assert(tree->size == 1000);

    for (int i = 0; i < 1000; i++) {
        char key[20];
        sprintf(key, "key%d", i);
        Vector *list = rbSearch(tree, key);
        assert(list != NULL && list->size == 1);
        PostingEntry *e = (PostingEntry *)getVectorItem(list, 0);
        assert(e != NULL && e->doc_id == i);
    }
    freeRBTree(tree);
}

int main(void) {
    test_create_free();
    test_insert_search_single();
    test_insert_duplicate();
    test_multiple_keys();
    test_traverse();
    test_rb_properties();
    test_large_insert();
    printf("All RBTree tests passed!\n");
    return 0;
}
