// Полностью на ваше усмотрение (только переиспользуйте код из предыдущих лабораторных, если он вам подходит)
// rbtree.c
#include "RBTree.h"
#include <stdlib.h>
#include <string.h>

static RBNode* createNode(const char* key, int doc_id, const char* title) {
    RBNode* node = (RBNode*)malloc(sizeof(RBNode));
    if (!node) return NULL;
    
    node->key = strdup(key);
    if (!node->key) {
        free(node);
        return NULL;
    }
    
    node->postings = createPostingList();
    if (!node->postings) {
        free(node->key);
        free(node);
        return NULL;
    }
    
    appendPosting(node->postings, doc_id, title);
    node->color = RB_RED;
    node->left = NULL;
    node->right = NULL;
    node->parent = NULL;
    
    return node;
}

static void freeNode(RBTree* tree, RBNode* node) {
    if (!node) return;
    freeNode(tree, node->left);
    freeNode(tree, node->right);
    free(node->key);
    if (node->postings) vectorFree(node->postings);
    free(node);
}

static void rotateLeft(RBTree* tree, RBNode* x) {
    RBNode* y = x->right;
    x->right = y->left;
    
    if (y->left != tree->nil)
        y->left->parent = x;
    
    y->parent = x->parent;
    
    if (x->parent == tree->nil)
        tree->root = y;
    else if (x == x->parent->left)
        x->parent->left = y;
    else
        x->parent->right = y;
    
    y->left = x;
    x->parent = y;
}

static void rotateRight(RBTree* tree, RBNode* y) {
    RBNode* x = y->left;
    y->left = x->right;
    
    if (x->right != tree->nil)
        x->right->parent = y;
    
    x->parent = y->parent;
    
    if (y->parent == tree->nil)
        tree->root = x;
    else if (y == y->parent->right)
        y->parent->right = x;
    else
        y->parent->left = x;
    
    x->right = y;
    y->parent = x;
}

static void insertFixup(RBTree* tree, RBNode* z) {
    while (z->parent->color == RB_RED) {
        if (z->parent == z->parent->parent->left) {
            RBNode* y = z->parent->parent->right;
            if (y->color == RB_RED) {
                z->parent->color = RB_BLACK;
                y->color = RB_BLACK;
                z->parent->parent->color = RB_RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->right) {
                    z = z->parent;
                    rotateLeft(tree, z);
                }
                z->parent->color = RB_BLACK;
                z->parent->parent->color = RB_RED;
                rotateRight(tree, z->parent->parent);
            }
        } else {
            RBNode* y = z->parent->parent->left;
            if (y->color == RB_RED) {
                z->parent->color = RB_BLACK;
                y->color = RB_BLACK;
                z->parent->parent->color = RB_RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->left) {
                    z = z->parent;
                    rotateRight(tree, z);
                }
                z->parent->color = RB_BLACK;
                z->parent->parent->color = RB_RED;
                rotateLeft(tree, z->parent->parent);
            }
        }
    }
    tree->root->color = RB_BLACK;
}

static void insertNode(RBTree* tree, const char* key, int doc_id, const char* title, int* is_new) {
    RBNode* z = createNode(key, doc_id, title);
    if (!z) {
        *is_new = 0;
        return;
    }
    
    RBNode* y = tree->nil;
    RBNode* x = tree->root;
    
    while (x != tree->nil) {
        y = x;
        int cmp = strcmp(key, x->key);
        if (cmp < 0) {
            x = x->left;
        } else if (cmp > 0) {
            x = x->right;
        } else {
            appendPosting(x->postings, doc_id, title);
            free(z->key);
            if (z->postings) vectorFree(z->postings);
            free(z);
            *is_new = 0;
            return;
        }
    }
    
    z->parent = y;
    
    if (y == tree->nil) {
        tree->root = z;
    } else if (strcmp(key, y->key) < 0) {
        y->left = z;
    } else {
        y->right = z;
    }
    
    z->left = tree->nil;
    z->right = tree->nil;
    z->color = RB_RED;
    
    insertFixup(tree, z);
    *is_new = 1;
}

RBTree* createRBTree(void) {
    RBTree* tree = (RBTree*)malloc(sizeof(RBTree));
    if (!tree) return NULL;
    
    tree->nil = (RBNode*)malloc(sizeof(RBNode));
    if (!tree->nil) {
        free(tree);
        return NULL;
    }
    
    tree->nil->color = RB_BLACK;
    tree->nil->left = NULL;
    tree->nil->right = NULL;
    tree->nil->parent = NULL;
    tree->nil->key = NULL;
    tree->nil->postings = NULL;
    
    tree->root = tree->nil;
    tree->size = 0;
    
    return tree;
}

void freeRBTree(RBTree* tree) {
    if (!tree) return;
    freeNode(tree, tree->root);
    free(tree->nil);
    free(tree);
}

void rbInsert(RBTree* tree, const char* key, int doc_id, const char* title) {
    if (!tree || !key || !title) return;
    int is_new = 0;
    insertNode(tree, key, doc_id, title, &is_new);
    if (is_new) tree->size++;
}

Vector* rbSearch(const RBTree* tree, const char* key) {
    if (!tree || !key) return NULL;
    
    RBNode* x = tree->root;
    while (x != tree->nil) {
        int cmp = strcmp(key, x->key);
        if (cmp == 0) return x->postings;
        if (cmp < 0) x = x->left;
        else x = x->right;
    }
    return NULL;
}

static void traverseInorder(RBTree* tree, RBNode* node,
                            void (*visit)(const char*, Vector*, void*),
                            void* ctx) {
    if (node == tree->nil) return;
    traverseInorder(tree, node->left, visit, ctx);
    visit(node->key, node->postings, ctx);
    traverseInorder(tree, node->right, visit, ctx);
}

void rbTraverse(const RBTree* tree,
                void (*visit)(const char*, Vector*, void*),
                void* ctx) {
    if (!tree || !visit) return;
    traverseInorder((RBTree*)tree, tree->root, visit, ctx);
}