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

    // Стек для итеративного post-order обхода (глубина B-дерева мала, но стек всё равно нужен)
    BTreeNode* stack[10000];  // хватит для 1000+ ключей
    int top = 0;
    BTreeNode* cur = node;
    BTreeNode* last_visited = NULL;

    while (cur || top > 0) {
        if (cur) {
            stack[top++] = cur;
            // если не лист — идём к первому ребёнку, иначе уходим в else
            cur = cur->is_leaf ? NULL : cur->children[0];
        } else {
            BTreeNode* peek = stack[top - 1];

            if (!peek->is_leaf) {
                // Ищем первого ещё не посещённого ребёнка
                int i;
                for (i = 0; i <= peek->n; i++) {
                    if (peek->children[i] == last_visited) {
                        // следующий за ним
                        if (i < peek->n) {
                            cur = peek->children[i + 1];
                            break;
                        }
                    }
                }
                // если нашли следующего ребёнка – продолжим с ним, иначе будем обрабатывать peek
                if (cur) continue;
            }

            // Обрабатываем узел peek
            for (int i = 0; i < peek->n; i++) {
                free(peek->keys[i]);
                if (peek->postings[i])
                    vectorFree(peek->postings[i]);
            }
            last_visited = peek;
            top--;
            free(peek);
        }
    }
}

/* Разбить полный дочерний узел child = parent->children[i] */
static void splitChild(BTreeNode* parent, int i, BTreeNode* child) {
    BTreeNode* z = createNode(child->is_leaf);
    int t = BTREE_T;
    z->n = t - 1;  /* число ключей в новом соседе */

    /* 1. Переместить последние t-1 ключей и posting-листов из child в z */
    for (int j = 0; j < t - 1; j++) {
        z->keys[j] = child->keys[j + t];
        child->keys[j + t] = NULL;   /* во избежание двойного освобождения */
        z->postings[j] = child->postings[j + t];
        child->postings[j + t] = NULL;
    }

    /* 2. Если child не лист, переместить последние t детей */
    if (!child->is_leaf) {
        for (int j = 0; j < t; j++) {
            z->children[j] = child->children[j + t];
            child->children[j + t] = NULL;
        }
    }

    child->n = t - 1;   /* после перемещения в child осталось t-1 ключей */

    /* 3. Освободить место в родителе для медианного ключа и нового ребёнка */
    for (int j = parent->n; j >= i + 1; j--) {
        parent->children[j + 1] = parent->children[j];
    }
    parent->children[i + 1] = z;

    for (int j = parent->n - 1; j >= i; j--) {
        parent->keys[j + 1] = parent->keys[j];
        parent->postings[j + 1] = parent->postings[j];
    }

    /* 4. Перенести медианный ключ (и его posting list) в родителя */
    parent->keys[i] = child->keys[t - 1];
    child->keys[t - 1] = NULL;        /* родитель теперь владеет этой строкой */
    parent->postings[i] = child->postings[t - 1];
    child->postings[t - 1] = NULL;
    parent->n++;
}

/* Вставка в неполный узел.
   Возвращает 0, если был вставлен новый ключ,
   1, если ключ уже существовал (добавлена запись в posting list). */
static int insertNonFull(BTreeNode* node, const char* key, int doc_id, const char* title) {
    int i = node->n - 1;

    if (node->is_leaf) {
        /* Поиск места вставки (проверка на существование ключа) */
        while (i >= 0 && strcmp(key, node->keys[i]) < 0)
            i--;
        if (i >= 0 && strcmp(key, node->keys[i]) == 0) {
            /* Ключ уже есть – добавить запись */
            appendPosting(node->postings[i], doc_id, title);
            return 1;   /* дубликат */
        }
        /* Сдвинуть элементы и вставить новый ключ */
        i++;  /* теперь i – позиция для нового ключа */
        for (int j = node->n; j > i; j--) {
            node->keys[j] = node->keys[j - 1];
            node->postings[j] = node->postings[j - 1];
        }
        node->keys[i] = strdup(key);
        node->postings[i] = createPostingList();
        appendPosting(node->postings[i], doc_id, title);
        node->n++;
        return 0;   /* вставлен новый уникальный ключ */
    }
    else {
        /* Найти ребёнка, в который надо спуститься */
        while (i >= 0 && strcmp(key, node->keys[i]) < 0)
            i--;
        i++;  /* индекс ребёнка */

        /* Если ребёнок полон – разбить его */
        if (node->children[i]->n == 2 * BTREE_T - 1) {
            splitChild(node, i, node->children[i]);
            /* После разбиения решаем, в левого или правого ребёнка идти */
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
    if (!tree) return;

    if (tree->root == NULL) {
        /* Первый узел – лист */
        tree->root = createNode(1);
        tree->root->keys[0] = strdup(key);
        tree->root->postings[0] = createPostingList();
        appendPosting(tree->root->postings[0], doc_id, title);
        tree->root->n = 1;
        tree->size = 1;
        return;
    }

    /* Если корень полон – создаём новый корень и разбиваем старый */
    if (tree->root->n == 2 * BTREE_T - 1) {
        BTreeNode* newRoot = createNode(0);
        newRoot->children[0] = tree->root;
        splitChild(newRoot, 0, tree->root);
        tree->root = newRoot;
    }

    int dup = insertNonFull(tree->root, key, doc_id, title);
    if (dup == 0)   /* новый уникальный ключ */
        tree->size++;
}

Vector* btreeSearch(const BTree* tree, const char* key) {
    if (!tree || !tree->root) return NULL;

    BTreeNode* node = tree->root;
    while (node) {
        int i = 0;
        while (i < node->n && strcmp(key, node->keys[i]) > 0)
            i++;
        if (i < node->n && strcmp(key, node->keys[i]) == 0)
            return node->postings[i];   /* нашли */
        if (node->is_leaf)
            return NULL;
        node = node->children[i];
    }
    return NULL;
}

/* Рекурсивный inorder-обход поддерева */
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