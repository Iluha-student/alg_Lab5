#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "index/index.h"
#include "index/search.h"

static const char* typeName(TreeType type) {
    switch (type) {
        case TREE_AVL:   return "avl";
        case TREE_RB:    return "rb";
        case TREE_BTREE: return "btree";
        default:         return "unknown";
    }
}

static TreeType parseType(const char* str) {
    if (strcmp(str, "avl") == 0)   return TREE_AVL;
    if (strcmp(str, "rb") == 0)    return TREE_RB;
    if (strcmp(str, "btree") == 0) return TREE_BTREE;
    return TREE_AVL;
}

static void runIndex(TreeType type, const char* data_path, const char* idx_path) {
    printf("Индексация типа %s...\n", typeName(type));
    
    FILE* f = fopen(data_path, "r");
    if (!f) {
        fprintf(stderr, "Ошибка: не удалось открыть %s\n", data_path);
        return;
    }
    
    Index* idx = createIndex(type);
    if (!idx) {
        fprintf(stderr, "Ошибка: не удалось создать индекс\n");
        fclose(f);
        return;
    }
    
    char line[65536];
    int doc_count = 0;
    
    while (fgets(line, sizeof(line), f)) {
        char* token_start = strstr(line, "\"doc_id\":");
        if (!token_start) continue;
        
        int doc_id = 0;
        if (sscanf(token_start, "\"doc_id\": %d", &doc_id) != 1) {
            if (sscanf(token_start, "\"doc_id\": \"%d\"", &doc_id) != 1) {
                continue;
            }
        }
        
        char title[512] = "";
        char* title_start = strstr(line, "\"title\":");
        if (title_start) {
            if (sscanf(title_start, "\"title\": \"%[^\"]\"", title) != 1) {
                continue;
            }
        }
        
        char* tokens_start = strstr(line, "\"tokens\":");
        if (!tokens_start) continue;
        
        char* bracket = strchr(tokens_start, '[');
        if (!bracket) continue;
     
        char* end_bracket = strchr(bracket, ']');
        if (!end_bracket) continue;
        
        char tokens_str[4096] = "";
        int len = end_bracket - bracket - 1;
        if (len > 0 && len < (int)sizeof(tokens_str)) {
            strncpy(tokens_str, bracket + 1, len);
            tokens_str[len] = '\0';
        }
        
        const char* token_ptrs[1000];
        char tokens_buf[1000][256];
        int token_count = 0;
        
        char* token = strtok(tokens_str, "\", []");
        while (token && token_count < 1000) {
            if (strlen(token) > 0) {
                strcpy(tokens_buf[token_count], token);
                token_ptrs[token_count] = tokens_buf[token_count];
                token_count++;
            }
            token = strtok(NULL, "\", []");
        }
        
        if (token_count > 0) {
            indexDocument(idx, doc_id, title, token_ptrs, token_count);
            doc_count++;
        }
        
        if (doc_count % 10000 == 0) {
            printf("Обработано документов: %d\n", doc_count);
        }
    }
    
    fclose(f);
    printf("Всего обработано: %d документов\n", doc_count);
    
    saveIndex(idx, idx_path);
    printf("Индекс сохранён в %s\n", idx_path);
    
    freeIndex(idx);
}

static void runSearch(TreeType type, const char* idx_path,
                      const char* query, int json_out) {
    printf("Поиск: '%s' в индексе %s\n", query, idx_path);
    
    Index* idx = loadIndex(idx_path, type);
    if (!idx) {
        fprintf(stderr, "Failed to load index: %s\n", idx_path);
        return;
    }

    SearchResults* sr = search(idx, query);
    if (json_out) printResultsJSON(sr);
    else          printResultsText(sr);

    freeSearchResults(sr);
    freeIndex(idx);
}

static void runFuzzySearch(TreeType type, const char* idx_path,
                           const char* query, int max_dist, int json_out) {
    printf("Нечёткий поиск: '%s' (max_dist=%d) в индексе %s\n", query, max_dist, idx_path);
    
    Index* idx = loadIndex(idx_path, type);
    if (!idx) {
        fprintf(stderr, "Failed to load index: %s\n", idx_path);
        return;
    }
    
    SearchResults* sr = fuzzySearch(idx, query, max_dist);
    if (json_out) printResultsJSON(sr);
    else          printResultsText(sr);
    
    freeSearchResults(sr);
    freeIndex(idx);
}

static void usage(const char* prog) {
    fprintf(stderr,
        "Usage:\n"
        "  %s index  --type=<avl|rb|btree> [--data=PATH] [--index=PATH]\n"
        "  %s search --type=<avl|rb|btree> [--index=PATH] [--json] \"query\"\n"
        "  %s fuzzy --type=<avl|rb|btree> [--index=PATH] [--max-dist=N] [--json] \"query\"\n",
        prog, prog, prog);
}

int main(int argc, char* argv[]) {
    if (argc < 3) { usage(argv[0]); return 1; }

    const char* mode = argv[1];
    TreeType    type = TREE_AVL;
    const char* data_path = "data/processed/docs.jsonl";
    char        idx_path[512] = {0};
    int         json_out = 0;
    const char* query    = NULL;
    int         max_dist = 2;   /* значение по умолчанию для нечёткого поиска */

    for (int i = 2; i < argc; i++) {
        if      (strncmp(argv[i], "--type=",  7) == 0) type = parseType(argv[i] + 7);
        else if (strncmp(argv[i], "--data=",  7) == 0) data_path = argv[i] + 7;
        else if (strncmp(argv[i], "--index=", 8) == 0)
            strncpy(idx_path, argv[i] + 8, sizeof(idx_path) - 1);
        else if (strncmp(argv[i], "--max-dist=", 11) == 0) max_dist = atoi(argv[i] + 11);
        else if (strcmp(argv[i], "--json")    == 0)    json_out = 1;
        else if (argv[i][0] != '-')                    query = argv[i];
    }

    if (idx_path[0] == '\0')
        snprintf(idx_path, sizeof(idx_path), "data/index_%s.txt", typeName(type));

    if (strcmp(mode, "index") == 0) {
        runIndex(type, data_path, idx_path);
    } else if (strcmp(mode, "search") == 0) {
        if (!query) { fprintf(stderr, "No query provided\n"); return 1; }
        runSearch(type, idx_path, query, json_out);
    } else if (strcmp(mode, "fuzzy") == 0) {
        if (!query) { fprintf(stderr, "No query provided\n"); return 1; }
        runFuzzySearch(type, idx_path, query, max_dist, json_out);
    } else {
        fprintf(stderr, "Unknown mode: %s\n", mode);
        usage(argv[0]);
        return 1;
    }
    return 0;
}
