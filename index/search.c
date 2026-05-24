#include "search.h"
#include "../posting.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

/* ---------- Вспомогательные функции для обычного поиска ---------- */

static int compare_search_results(const void* a, const void* b) {
    const SearchResult* ra = (const SearchResult*)a;
    const SearchResult* rb = (const SearchResult*)b;
    return rb->score - ra->score;
}

static void add_to_results(Vector* results, int doc_id, const char* title) {
    for (size_t i = 0; i < results->size; i++) {
        SearchResult* r = (SearchResult*)getVectorItem(results, i);
        if (r->doc_id == doc_id) {
            r->score++;
            return;
        }
    }
    
    SearchResult new_result;
    new_result.doc_id = doc_id;
    strncpy(new_result.title, title, MAX_TITLE_LEN - 1);
    new_result.title[MAX_TITLE_LEN - 1] = '\0';
    new_result.score = 1;
    appendVectorItem(results, &new_result);
}

static Vector* tokenize_query(const char* query) {
    Vector* tokens = createVector(sizeof(char*));
    if (!tokens) return NULL;
    
    char* query_copy = strdup(query);
    if (!query_copy) return tokens;
    
    char* token = strtok(query_copy, " ");
    while (token) {
        for (char* p = token; *p; p++) {
            *p = tolower(*p);
        }
        char* token_copy = strdup(token);
        if (token_copy) {
            appendVectorItem(tokens, &token_copy);
        }
        token = strtok(NULL, " ");
    }
    
    free(query_copy);
    return tokens;
}

Vector* intersectPostings(Vector** lists, int n) {
    if (!lists || n <= 0) return NULL;
    
    Vector* result = createPostingList();
    if (!result) return NULL;
    
    for (int i = 0; i < n; i++) {
        if (!lists[i] || lists[i]->size == 0) {
            vectorFree(result);
            return NULL;
        }
    }
    
    size_t* indices = (size_t*)calloc(n, sizeof(size_t));
    if (!indices) {
        vectorFree(result);
        return NULL;
    }
    
    while (1) {
        int max_doc_id = -1;
        for (int i = 0; i < n; i++) {
            PostingEntry* entry = (PostingEntry*)getVectorItem(lists[i], indices[i]);
            if (entry->doc_id > max_doc_id) {
                max_doc_id = entry->doc_id;
            }
        }
        
        int all_match = 1;
        for (int i = 0; i < n; i++) {
            PostingEntry* entry = (PostingEntry*)getVectorItem(lists[i], indices[i]);
            if (entry->doc_id != max_doc_id) {
                all_match = 0;
                break;
            }
        }
        
        if (all_match) {
            PostingEntry* entry = (PostingEntry*)getVectorItem(lists[0], indices[0]);
            appendPosting(result, entry->doc_id, entry->title);
            
            int advance_all = 1;
            for (int i = 0; i < n; i++) {
                indices[i]++;
                if (indices[i] >= lists[i]->size) {
                    advance_all = 0;
                    break;
                }
            }
            if (!advance_all) break;
        } else {
            for (int i = 0; i < n; i++) {
                PostingEntry* entry = (PostingEntry*)getVectorItem(lists[i], indices[i]);
                if (entry->doc_id != max_doc_id) {
                    indices[i]++;
                    if (indices[i] >= lists[i]->size) {
                        free(indices);
                        return result;
                    }
                }
            }
        }
    }
    
    free(indices);
    return result;
}

SearchResults* search(Index* idx, const char* query) {
    if (!idx || !query) return NULL;
    
    clock_t start = clock();
    
    SearchResults* sr = (SearchResults*)malloc(sizeof(SearchResults));
    if (!sr) return NULL;
    
    sr->results = createVector(sizeof(SearchResult));
    sr->total = 0;
    
    Vector* tokens = tokenize_query(query);
    if (!tokens || tokens->size == 0) {
        sr->time_ms = 0;
        if (tokens) vectorFree(tokens);
        return sr;
    }
    
    Vector** posting_lists = (Vector**)malloc(tokens->size * sizeof(Vector*));
    int valid_lists = 0;
    
    for (size_t i = 0; i < tokens->size; i++) {
        char** token_ptr = (char**)getVectorItem(tokens, i);
        Vector* postings = lookupTerm(idx, *token_ptr);
        
        if (postings && postings->size > 0) {
            posting_lists[valid_lists++] = postings;
        } else {
            if (postings) vectorFree(postings);
            for (int j = 0; j < valid_lists; j++) {
                vectorFree(posting_lists[j]);
            }
            for (size_t j = 0; j < tokens->size; j++) {
                char** t = (char**)getVectorItem(tokens, j);
                free(*t);
            }
            vectorFree(tokens);
            free(posting_lists);
            sr->time_ms = 0;
            return sr;
        }
    }
    
    Vector* intersection = NULL;
    if (valid_lists == 1) {
        intersection = clonePostingList(posting_lists[0]);
    } else {
        intersection = intersectPostings(posting_lists, valid_lists);
    }
    
    if (intersection) {
        sr->total = intersection->size;
        for (size_t i = 0; i < intersection->size && i < 10; i++) {
            PostingEntry* entry = (PostingEntry*)getVectorItem(intersection, i);
            SearchResult result;
            result.doc_id = entry->doc_id;
            strncpy(result.title, entry->title, MAX_TITLE_LEN - 1);
            result.title[MAX_TITLE_LEN - 1] = '\0';
            result.score = 1;
            appendVectorItem(sr->results, &result);
        }
        vectorFree(intersection);
    }
    
    for (int i = 0; i < valid_lists; i++) {
        vectorFree(posting_lists[i]);
    }
    for (size_t i = 0; i < tokens->size; i++) {
        char** t = (char**)getVectorItem(tokens, i);
        free(*t);
    }
    vectorFree(tokens);
    free(posting_lists);
    
    clock_t end = clock();
    sr->time_ms = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    
    return sr;
}

void printResultsText(const SearchResults* sr) {
    if (!sr) {
        printf("No results.\n");
        return;
    }
    printf("Found %d documents (%.2f ms):\n", sr->total, sr->time_ms);
    printf("Top %zu results:\n", sr->results->size);
    for (size_t i = 0; i < sr->results->size; i++) {
        SearchResult* r = (SearchResult*)getVectorItem(sr->results, i);
        printf("%zu. [%d] %s (score: %d)\n", i + 1, r->doc_id, r->title, r->score);
    }
}

void printResultsJSON(const SearchResults* sr) {
    if (!sr) {
        printf("{\"error\": \"no results\"}\n");
        return;
    }
    printf("{\n");
    printf("  \"total\": %d,\n", sr->total);
    printf("  \"time_ms\": %.2f,\n", sr->time_ms);
    printf("  \"results\": [\n");
    for (size_t i = 0; i < sr->results->size; i++) {
        SearchResult* r = (SearchResult*)getVectorItem(sr->results, i);
        printf("    {\"doc_id\": %d, \"title\": \"%s\", \"score\": %d}",
               r->doc_id, r->title, r->score);
        if (i < sr->results->size - 1) printf(",");
        printf("\n");
    }
    printf("  ]\n");
    printf("}\n");
}

void freeSearchResults(SearchResults* sr) {
    if (!sr) return;
    if (sr->results) vectorFree(sr->results);
    free(sr);
}

/* ---------- Функции нечёткого поиска (задача 4) ---------- */

static int min3(int a, int b, int c) {
    int m = a < b ? a : b;
    return m < c ? m : c;
}

/* Расстояние Левенштейна с ранним выходом */
static int levenshtein_distance(const char* s, const char* t, int max_dist) {
    int n = strlen(s);
    int m = strlen(t);
    if (abs(n - m) > max_dist) return max_dist + 1;
    
    int* prev = malloc((m + 1) * sizeof(int));
    int* curr = malloc((m + 1) * sizeof(int));
    if (!prev || !curr) {
        free(prev); free(curr);
        return max_dist + 1;
    }
    
    for (int j = 0; j <= m; j++) prev[j] = j;
    
    for (int i = 1; i <= n; i++) {
        curr[0] = i;
        int min_row = curr[0];
        for (int j = 1; j <= m; j++) {
            int cost = (s[i-1] == t[j-1]) ? 0 : 1;
            curr[j] = min3(prev[j] + 1, curr[j-1] + 1, prev[j-1] + cost);
            if (curr[j] < min_row) min_row = curr[j];
        }
        if (min_row > max_dist) {
            free(prev); free(curr);
            return max_dist + 1;
        }
        int* tmp = prev;
        prev = curr;
        curr = tmp;
    }
    int result = prev[m];
    free(prev); free(curr);
    return result;
}

/* Контекст для обхода дерева при сборе кандидатов */
typedef struct {
    Vector* candidates;
    const char* term;
    int max_distance;
} FuzzyCtx;

static void fuzzy_collect_candidate(const char* key, Vector* postings, void* user) {
    FuzzyCtx* ctx = (FuzzyCtx*)user;
    int dist = levenshtein_distance(ctx->term, key, ctx->max_distance);
    if (dist <= ctx->max_distance) {
        FuzzyCandidate fc;
        strncpy(fc.term, key, sizeof(fc.term) - 1);
        fc.term[sizeof(fc.term)-1] = '\0';
        fc.distance = dist;
        fc.postings = clonePostingList(postings);
        if (fc.postings) {
            appendVectorItem(ctx->candidates, &fc);
        }
    }
}

Vector* fuzzyFindCandidates(Index* idx, const char* term, int max_distance) {
    if (!idx || !term) return NULL;
    Vector* candidates = createVector(sizeof(FuzzyCandidate));
    if (!candidates) return NULL;
    FuzzyCtx ctx = {candidates, term, max_distance};
    traverseIndex(idx, fuzzy_collect_candidate, &ctx);
    return candidates;
}

SearchResults* fuzzySearch(Index* idx, const char* query, int max_distance) {
    if (!idx || !query) return NULL;
    
    clock_t start = clock();
    
    Vector* tokens = tokenize_query(query);
    if (!tokens || tokens->size == 0) {
        if (tokens) vectorFree(tokens);
        SearchResults* sr = calloc(1, sizeof(SearchResults));
        sr->results = createVector(sizeof(SearchResult));
        sr->time_ms = 0.0;
        return sr;
    }
    
    Vector* all_results = createVector(sizeof(SearchResult));
    
    for (size_t i = 0; i < tokens->size; i++) {
        char** token_ptr = (char**)getVectorItem(tokens, i);
        const char* term = *token_ptr;
        Vector* cands = fuzzyFindCandidates(idx, term, max_distance);
        if (!cands) continue;
        
        for (size_t j = 0; j < cands->size; j++) {
            FuzzyCandidate* fc = (FuzzyCandidate*)getVectorItem(cands, j);
            int term_score = 10 - fc->distance;   // чем меньше расстояние, тем выше вклад
            
            for (size_t k = 0; k < fc->postings->size; k++) {
                PostingEntry* pe = (PostingEntry*)getVectorItem(fc->postings, k);
                int found = -1;
                for (size_t r = 0; r < all_results->size; r++) {
                    SearchResult* sr = (SearchResult*)getVectorItem(all_results, r);
                    if (sr->doc_id == pe->doc_id) {
                        found = r;
                        break;
                    }
                }
                if (found >= 0) {
                    SearchResult* sr = (SearchResult*)getVectorItem(all_results, found);
                    sr->score += term_score;
                } else {
                    SearchResult new_sr;
                    new_sr.doc_id = pe->doc_id;
                    strncpy(new_sr.title, pe->title, MAX_TITLE_LEN - 1);
                    new_sr.title[MAX_TITLE_LEN - 1] = '\0';
                    new_sr.score = term_score;
                    appendVectorItem(all_results, &new_sr);
                }
            }
            vectorFree(fc->postings);
        }
        vectorFree(cands);
    }
    
    /* Сортировка по убыванию score (пузырьком, для простоты) */
    for (size_t i = 0; i < all_results->size; i++) {
        for (size_t j = i + 1; j < all_results->size; j++) {
            SearchResult* a = (SearchResult*)getVectorItem(all_results, i);
            SearchResult* b = (SearchResult*)getVectorItem(all_results, j);
            if (a->score < b->score) {
                SearchResult tmp = *a;
                *a = *b;
                *b = tmp;
            }
        }
    }
    
    SearchResults* sr = malloc(sizeof(SearchResults));
    sr->total = all_results->size;
    sr->results = createVector(sizeof(SearchResult));
    for (size_t i = 0; i < all_results->size && i < 10; i++) {
        SearchResult* src = (SearchResult*)getVectorItem(all_results, i);
        appendVectorItem(sr->results, src);
    }
    
    /* Очистка */
    for (size_t i = 0; i < tokens->size; i++) {
        char** t = (char**)getVectorItem(tokens, i);
        free(*t);
    }
    vectorFree(tokens);
    vectorFree(all_results);
    
    clock_t end = clock();
    sr->time_ms = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    return sr;
}
