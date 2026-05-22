#include "rbtree.h"

RBTree* createRBTree(void) {
    RBTree* rbtree = malloc(sizeof(RBTree));
    if (!rbtree) return NULL;

    rbtree->root = malloc(sizeof(RBNode));
    if (!rbtree->root) return NULL;

    rbtree->nil = malloc(sizeof(RBNode));
    if (!rbtree->nil) return NULL;

    rbtree->size = 1;

    return rbtree;   
}

void freeRBTree(RBTree* tree) {
    return;
}

void rbInsert(RBTree* tree, const char* key, int doc_id, const char* title) {
    tree->size++;
    PostingEntry post = {doc_id, title};
    return;
}

Vector* rbSearch(const RBTree* tree, const char* key) {
    return NULL;
}

void rbTraverse(
    const RBTree* tree,
    void (*visit)(const char* key, Vector* postings, void* ctx),
    void* ctx
)
{
    return;
}
