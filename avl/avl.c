#define _POSIX_C_SOURCE 200809L
#include "avl.h"
#include <stdlib.h>
#include <string.h>

// ========== Вспомогательные функции ==========

static int height(AVLNode *node) {
    return node ? node->height : 0;
}

static void updateHeight(AVLNode *node) {
    if (node) {
        int leftH = height(node->left);
        int rightH = height(node->right);
        node->height = (leftH > rightH ? leftH : rightH) + 1;
    }
}

static int balanceFactor(AVLNode *node) {
    return node ? height(node->left) - height(node->right) : 0;
}

static AVLNode *rotateRight(AVLNode *y) {
    AVLNode *x = y->left;
    AVLNode *T2 = x->right;

    x->right = y;
    y->left = T2;

    updateHeight(y);
    updateHeight(x);
    return x;
}

static AVLNode *rotateLeft(AVLNode *x) {
    AVLNode *y = x->right;
    AVLNode *T2 = y->left;

    y->left = x;
    x->right = T2;

    updateHeight(x);
    updateHeight(y);
    return y;
}

static AVLNode *balance(AVLNode *node) {
    if (!node) return NULL;
    updateHeight(node);

    int bf = balanceFactor(node);
    if (bf > 1) {
        if (balanceFactor(node->left) < 0)
            node->left = rotateLeft(node->left);
        return rotateRight(node);
    }
    if (bf < -1) {
        if (balanceFactor(node->right) > 0)
            node->right = rotateRight(node->right);
        return rotateLeft(node);
    }
    return node;
}

// ========== Создание / удаление узлов ==========

static AVLNode *createNode(const char *key, int doc_id, const char *title) {
    AVLNode *node = malloc(sizeof(AVLNode));
    if (!node) return NULL;

    node->key = strdup(key);
    if (!node->key) {
        free(node);
        return NULL;
    }

    node->left = node->right = NULL;
    node->height = 1;
    node->postings = createPostingList();
    if (!node->postings) {
        free(node->key);
        free(node);
        return NULL;
    }

    appendPosting(node->postings, doc_id, title);
    return node;
}

static void freeNode(AVLNode *node) {
    if (!node) return;
    freeNode(node->left);
    freeNode(node->right);
    free(node->key);
    if (node->postings) vectorFree(node->postings);
    free(node);
}

// ========== Рекурсивная вставка ==========

static AVLNode *insertNode(AVLNode *node, const char *key,
                           int doc_id, const char *title, int *is_new) {
    if (!node) {
        AVLNode *new_node = createNode(key, doc_id, title);
        if (!new_node) {
            *is_new = 0;
            return NULL;
        }
        *is_new = 1;
        return new_node;
    }

    int cmp = strcmp(key, node->key);
    if (cmp < 0)
        node->left = insertNode(node->left, key, doc_id, title, is_new);
    else if (cmp > 0)
        node->right = insertNode(node->right, key, doc_id, title, is_new);
    else {
        appendPosting(node->postings, doc_id, title);
        *is_new = 0;
        return node;
    }

    return balance(node);
}

// ========== Публичные функции ==========

AVLTree *createAVLTree(void) {
    AVLTree *tree = malloc(sizeof(AVLTree));
    if (!tree) return NULL;
    tree->root = NULL;
    tree->size = 0;
    return tree;
}

void freeAVLTree(AVLTree *tree) {
    if (!tree) return;
    freeNode(tree->root);
    free(tree);
}

void avlInsert(AVLTree *tree, const char *key, int doc_id, const char *title) {
    if (!tree || !key || !title) return;
    int is_new = 0;
    tree->root = insertNode(tree->root, key, doc_id, title, &is_new);
    if (is_new) tree->size++;
}

Vector *avlSearch(const AVLTree *tree, const char *key) {
    if (!tree || !key) return NULL;
    AVLNode *cur = tree->root;
    while (cur) {
        int cmp = strcmp(key, cur->key);
        if (cmp == 0) return cur->postings;
        if (cmp < 0) cur = cur->left;
        else cur = cur->right;
    }
    return NULL;
}

static void traverseInorder(AVLNode *node,
                            void (*visit)(const char *, Vector *, void *),
                            void *ctx) {
    if (!node) return;
    traverseInorder(node->left, visit, ctx);
    visit(node->key, node->postings, ctx);
    traverseInorder(node->right, visit, ctx);
}

void avlTraverse(const AVLTree *tree,
                 void (*visit)(const char *, Vector *, void *),
                 void *ctx) {
    if (!tree || !visit) return;
    traverseInorder(tree->root, visit, ctx);
}
