#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "btree.h"

/* ----- Вспомогательные функции ----- */

static BTreeNode* createNode(int is_leaf) {
    BTreeNode* node = malloc(sizeof(BTreeNode));
    if (!node) return NULL;
    node->n = 0;
    node->is_leaf = is_leaf;
    for (int i = 0; i < BTREE_MAX_KEYS; i++) {
        node->keys[i] = NULL;
        node->postings[i] = NULL;
    }
    for (int i = 0; i < BTREE_MAX_CH; i++) {
        node->children[i] = NULL;
    }
    return node;
}

static void freeNode(BTreeNode* node) {
    if (!node) return;

    // Простой рекурсивный обход (без стека)
    // Для B-дерева глубина невелика, можно использовать рекурсию
    if (!node->is_leaf) {
        for (int i = 0; i <= node->n; i++) {
            freeNode(node->children[i]);
        }
    }
    for (int i = 0; i < node->n; i++) {
        free(node->keys[i]);
        if (node->postings[i]) vectorFree(node->postings[i]);
    }
    free(node);
}

/* Разбить полный дочерний узел child = parent->children[i] */
static void splitChild(BTreeNode* parent, int i, BTreeNode* child) {
    BTreeNode* z = createNode(child->is_leaf);
    int t = BTREE_T;
    z->n = t - 1;

    for (int j = 0; j < t - 1; j++) {
        z->keys[j] = child->keys[j + t];
        child->keys[j + t] = NULL;
        z->postings[j] = child->postings[j + t];
        child->postings[j + t] = NULL;
    }

    if (!child->is_leaf) {
        for (int j = 0; j < t; j++) {
            z->children[j] = child->children[j + t];
            child->children[j + t] = NULL;
        }
    }

    child->n = t - 1;

    for (int j = parent->n; j >= i + 1; j--) {
        parent->children[j + 1] = parent->children[j];
    }
    parent->children[i + 1] = z;

    for (int j = parent->n - 1; j >= i; j--) {
        parent->keys[j + 1] = parent->keys[j];
        parent->postings[j + 1] = parent->postings[j];
    }

    parent->keys[i] = child->keys[t - 1];
    child->keys[t - 1] = NULL;
    parent->postings[i] = child->postings[t - 1];
    child->postings[t - 1] = NULL;
    parent->n++;
}

/* Вставка в неполный узел */
static int insertNonFull(BTreeNode* node, const char* key, int doc_id, const char* title) {
    int i = node->n - 1;

    if (node->is_leaf) {
        while (i >= 0 && strcmp(key, node->keys[i]) < 0)
            i--;
        if (i >= 0 && strcmp(key, node->keys[i]) == 0) {
            appendPosting(node->postings[i], doc_id, title);
            return 1;
        }
        i++;
        for (int j = node->n; j > i; j--) {
            node->keys[j] = node->keys[j - 1];
            node->postings[j] = node->postings[j - 1];
        }
        node->keys[i] = strdup(key);
        if (!node->keys[i]) return 0; // ошибка выделения
        node->postings[i] = createPostingList();
        if (!node->postings[i]) {
            free(node->keys[i]);
            node->keys[i] = NULL;
            return 0;
        }
        appendPosting(node->postings[i], doc_id, title);
        node->n++;
        return 0;
    } else {
        while (i >= 0 && strcmp(key, node->keys[i]) < 0)
            i--;
        i++;
        if (node->children[i]->n == 2 * BTREE_T - 1) {
            splitChild(node, i, node->children[i]);
            if (strcmp(key, node->keys[i]) > 0)
                i++;
        }
        return insertNonFull(node->children[i], key, doc_id, title);
    }
}

/* ----- Публичные функции ----- */

BTree* createBTree(void) {
    BTree* tree = malloc(sizeof(BTree));
    if (!tree) return NULL;
    tree->root = NULL;
    tree->size = 0;
    return tree;
}

void freeBTree(BTree* tree) {
    if (!tree) return;
    freeNode(tree->root);
    free(tree);
}

void btreeInsert(BTree* tree, const char* key, int doc_id, const char* title) {
    if (!tree || !key || !title) return;

    if (tree->root == NULL) {
        tree->root = createNode(1);
        if (!tree->root) return;
        tree->root->keys[0] = strdup(key);
        if (!tree->root->keys[0]) {
            freeNode(tree->root);
            tree->root = NULL;
            return;
        }
        tree->root->postings[0] = createPostingList();
        if (!tree->root->postings[0]) {
            free(tree->root->keys[0]);
            freeNode(tree->root);
            tree->root = NULL;
            return;
        }
        appendPosting(tree->root->postings[0], doc_id, title);
        tree->root->n = 1;
        tree->size = 1;
        return;
    }

    if (tree->root->n == 2 * BTREE_T - 1) {
        BTreeNode* newRoot = createNode(0);
        if (!newRoot) return;
        newRoot->children[0] = tree->root;
        splitChild(newRoot, 0, tree->root);
        tree->root = newRoot;
    }

    int dup = insertNonFull(tree->root, key, doc_id, title);
    if (dup == 0)
        tree->size++;
}

Vector* btreeSearch(const BTree* tree, const char* key) {
    if (!tree || !tree->root || !key) return NULL;

    BTreeNode* node = tree->root;
    while (node) {
        int i = 0;
        while (i < node->n && strcmp(key, node->keys[i]) > 0)
            i++;
        if (i < node->n && strcmp(key, node->keys[i]) == 0)
            return node->postings[i];
        if (node->is_leaf)
            return NULL;
        node = node->children[i];
    }
    return NULL;
}

static void traverseNode(BTreeNode* node,
                         void (*visit)(const char* key, Vector* postings, void* ctx),
                         void* ctx) {
    if (!node) return;
    int i;
    for (i = 0; i < node->n; i++) {
        if (!node->is_leaf)
            traverseNode(node->children[i], visit, ctx);
        visit(node->keys[i], node->postings[i], ctx);
    }
    if (!node->is_leaf)
        traverseNode(node->children[node->n], visit, ctx);
}

void btreeTraverse(const BTree* tree,
                   void (*visit)(const char* key, Vector* postings, void* ctx),
                   void* ctx) {
    if (!tree || !tree->root || !visit) return;
    traverseNode(tree->root, visit, ctx);
}
