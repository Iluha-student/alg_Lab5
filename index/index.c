// Полностью на ваше усмотрение (только переиспользуйте код из предыдущих лабораторных, если он вам подходит)
#include "index.h"
#include "../avl/avl.h"
#include "../rbtree/rbtree.h"
#include "../btree/btree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Index* createIndex(TreeType type) {
    Index* idx = (Index*)malloc(sizeof(Index));
    if (!idx) return NULL;
    
    idx->type = type;
    
    switch (type) {
        case TREE_AVL:
            idx->tree = createAVLTree();
            break;
        case TREE_RB:
            idx->tree = createRBTree();
            break;
        case TREE_BTREE:
            idx->tree = createBTree();
            break;
        default:
            free(idx);
            return NULL;
    }
    
    return idx;
}

void insertTerm(Index* idx, const char* term, int doc_id, const char* title) {
    if (!idx || !term || !title) return;
    
    switch (idx->type) {
        case TREE_AVL:
            avlInsert((AVLTree*)idx->tree, term, doc_id, title);
            break;
        case TREE_RB:
            rbInsert((RBTree*)idx->tree, term, doc_id, title);
            break;
        case TREE_BTREE:
            btreeInsert((BTree*)idx->tree, term, doc_id, title);
            break;
    }
}

Vector* lookupTerm(const Index* idx, const char* term) {
    if (!idx || !term) return NULL;
    
    Vector* original = NULL;
    
    switch (idx->type) {
        case TREE_AVL:
            original = avlSearch((AVLTree*)idx->tree, term);
            break;
        case TREE_RB:
            original = rbSearch((RBTree*)idx->tree, term);
            break;
        case TREE_BTREE:
            original = btreeSearch((BTree*)idx->tree, term);
            break;
    }
    
    if (original) {
        return clonePostingList(original);
    }
    
    return NULL;
}

void indexDocument(Index* idx, int doc_id, const char* title,
                   const char** tokens, int n_tokens) {
    if (!idx || !title || !tokens || n_tokens <= 0) return;
    
    for (int i = 0; i < n_tokens; i++) {
        if (tokens[i] && tokens[i][0] != '\0') {
            insertTerm(idx, tokens[i], doc_id, title);
        }
    }
}

static void save_callback(const char* key, Vector* postings, void* ctx) {
    FILE* file = (FILE*)ctx;
    if (!file || !key || !postings) return;
    
    fprintf(file, "%s:", key);
    
    for (size_t i = 0; i < postings->size; i++) {
        PostingEntry* entry = (PostingEntry*)getVectorItem(postings, i);
        fprintf(file, " %d,%s", entry->doc_id, entry->title);
    }
    fprintf(file, "\n");
}

void saveIndex(const Index* idx, const char* path) {
    if (!idx || !path) return;
    
    FILE* file = fopen(path, "w");
    if (!file) {
        fprintf(stderr, "Error: cannot open file %s for writing\n", path);
        return;
    }
    
    traverseIndex(idx, save_callback, file);
    
    fclose(file);
}

Index* loadIndex(const char* path, TreeType type) {
    if (!path) return NULL;
    
    FILE* file = fopen(path, "r");
    if (!file) {
        fprintf(stderr, "Error: cannot open file %s for reading\n", path);
        return NULL;
    }
    
    Index* idx = createIndex(type);
    if (!idx) {
        fclose(file);
        return NULL;
    }
    
    char line[65536];
    
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0';
        
        char* colon = strchr(line, ':');
        if (!colon) continue;
        
        *colon = '\0';
        char* key = line;
        char* postings_str = colon + 1;
        
        char* token = strtok(postings_str, " ");
        while (token) {
            int doc_id;
            char title[256];
            
            if (sscanf(token, "%d,%255s", &doc_id, title) == 2) {
                insertTerm(idx, key, doc_id, title);
            }
            
            token = strtok(NULL, " ");
        }
    }
    
    fclose(file);
    return idx;
}

void traverseIndex(const Index* idx,
                   void (*visit)(const char* key, Vector* postings, void* ctx),
                   void* ctx) {
    if (!idx || !visit) return;
    
    switch (idx->type) {
        case TREE_AVL:
            avlTraverse((AVLTree*)idx->tree, visit, ctx);
            break;
        case TREE_RB:
            rbTraverse((RBTree*)idx->tree, visit, ctx);
            break;
        case TREE_BTREE:
            btreeTraverse((BTree*)idx->tree, visit, ctx);
            break;
    }
}

void freeIndex(Index* idx) {
    if (!idx) return;
    
    switch (idx->type) {
        case TREE_AVL:
            freeAVLTree((AVLTree*)idx->tree);
            break;
        case TREE_RB:
            freeRBTree((RBTree*)idx->tree);
            break;
        case TREE_BTREE:
            freeBTree((BTree*)idx->tree);
            break;
    }
    
    free(idx);
}
