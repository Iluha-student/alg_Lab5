#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#include "generic.h"

void test_empty_table() {
    printf("Testing empty table...\n");
    HashTable* table = createHashTable(sizeof(int), sizeof(int));
    assert(table != NULL);
    
    int key = 42;
    assert(getItemHashTable(table, &key, HashInt) == NULL);
    assert(popItemHashTable(table, &key, HashInt) == NULL);
    
    freeHashTable(table);
    printf("Empty table test passed!\n");
}

void test_single_element() {
    printf("Testing single element...\n");
    HashTable* table = createHashTable(sizeof(int), sizeof(int));
    
    int key = 42, value = 100;
    setItemHashTable(table, &key, &value, HashInt);
    assert(table->size == 1);
    
    int* retrieved = (int*)getItemHashTable(table, &key, HashInt);
    assert(retrieved != NULL && *retrieved == value);
    
    freeHashTable(table);
    printf("Single element test passed!\n");
}

void test_collisions() {
    printf("Testing collisions...\n");
    HashTable* table = createHashTable(sizeof(char*), sizeof(int));
    
    char *key1 = "ab", *key2 = "ba";
    int val1 = 100, val2 = 200;
    
    setItemHashTable(table, key1, &val1, HashString);
    setItemHashTable(table, key2, &val2, HashString);
    assert(table->size == 2);
    
    int* retrieved1 = (int*)getItemHashTable(table, key1, HashString);
    int* retrieved2 = (int*)getItemHashTable(table, key2, HashString);
    assert(retrieved1 != NULL && *retrieved1 == val1);
    assert(retrieved2 != NULL && *retrieved2 == val2);
    
    freeHashTable(table);
    printf("Collision test passed!\n");
}

void test_rehash() {
    printf("Testing rehash...\n");
    HashTable* table = createHashTable(sizeof(int), sizeof(int));
    
    // Вставляем несколько элементов для вызова rehash
    for (int i = 0; i < 10; i++) {
        setItemHashTable(table, &i, &i, HashInt);
    }
    
    // Проверяем, что все элементы доступны
    for (int i = 0; i < 10; i++) {
        int* retrieved = (int*)getItemHashTable(table, &i, HashInt);
        assert(retrieved != NULL && *retrieved == i);
    }
    
    freeHashTable(table);
    printf("Rehash test passed!\n");
}

void test_delete_and_reinsert() {
    printf("Testing delete and reinsert...\n");
    HashTable* table = createHashTable(sizeof(int), sizeof(int));
    
    int key = 42, value = 100;
    setItemHashTable(table, &key, &value, HashInt);
    
    int* deleted = (int*)popItemHashTable(table, &key, HashInt);
    assert(deleted != NULL && *deleted == value);
    free(deleted);
    assert(table->size == 0);
    
    int new_value = 200;
    setItemHashTable(table, &key, &new_value, HashInt);
    int* retrieved = (int*)getItemHashTable(table, &key, HashInt);
    assert(retrieved != NULL && *retrieved == new_value);
    
    freeHashTable(table);
    printf("Delete and reinsert test passed!\n");
}

void test_invalid_input() {
    printf("Testing invalid input...\n");
    HashTable* table = createHashTable(sizeof(int), sizeof(int));
    
    int key = 42, value = 100;
    setItemHashTable(NULL, &key, &value, HashInt); // Не падает
    setItemHashTable(table, NULL, &value, HashInt); // Не падает
    
    assert(getItemHashTable(NULL, &key, HashInt) == NULL);
    assert(popItemHashTable(NULL, &key, HashInt) == NULL);
    
    HashTable* bad_table = createHashTable(0, sizeof(int));
    assert(bad_table == NULL);
    
    freeHashTable(table);
    printf("Invalid input test passed!\n");
}

void stress_test_performance() {
    printf("Testing performance...\n");
    clock_t start = clock();
    
    HashTable* table = createHashTable(sizeof(int), sizeof(int));
    
    // Вставка 100_000 элементов (уменьшено для быстроты тестов)
    for (int i = 0; i < 100000; i++) {
        setItemHashTable(table, &i, &i, HashInt);
    }
    
    // Получение 100_000 элементов
    for (int i = 0; i < 100000; i++) {
        int* retrieved = (int*)getItemHashTable(table, &i, HashInt);
        assert(retrieved != NULL && *retrieved == i);
    }
    
    clock_t end = clock();
    double time_spent = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Time for 100_000 operations: %.4f seconds\n", time_spent);

    // print_all_keys(table);

    freeHashTable(table);
    printf("Performance test passed!\n");
}

int main() {
    printf("\u001B[32mRunning HashTable tests...\u001B[0m\n\n");
    
    test_empty_table();
    printf("\n");
    
    test_single_element();
    printf("\n");
    
    test_collisions();
    printf("\n");
    
    test_rehash();
    printf("\n");
    
    test_delete_and_reinsert();
    printf("\n");
    
    test_invalid_input();
    printf("\n");
    
    stress_test_performance();
    printf("\n");

    printf("\n\u001B[33mFile collision test:\u001B[0m\n");
    
    // Тест с хорошими случаями
    FILE *file = fopen("data/good_cases.csv", "r");
    if (file == NULL) {
        printf("\nFile open error for good_cases.csv!\n");
    } else {
        HashTable* table = createHashTable(sizeof(char) * 120, sizeof(int));
        
        char str[120];
        while (fscanf(file, "%s", str) == 1) {
            int* cnt = (int*)getItemHashTable(table, str, HashString);
            if (cnt == NULL) {
                int value = 1;
                setItemHashTable(table, str, &value, HashString);
            } else {
                (*cnt)++;
            }
        }
        
        printf("\nCollision count (good cases): %lu", getCollisionCount(table, HashString));
        printf("\nCollision count (fast) (good cases): %lu\n", getCollisionCountFast(table));
        printf("\nCapacity: %d\n", (int)table->capacity);
        
        fclose(file);
        freeHashTable(table);
    }

    // Тест с плохими случаями
    FILE *bad_file = fopen("data/bad_cases.csv", "r");
    if (bad_file == NULL) {
        printf("\nFile open error for bad_cases.csv!\n");
    } else {
        HashTable* bad_table = createHashTable(sizeof(char) * 120, sizeof(int));
        
        char bad_str[120];
        while (fscanf(bad_file, "%s", bad_str) == 1) {
            int* bad_cnt = (int*)getItemHashTable(bad_table, bad_str, HashString);
            if (bad_cnt == NULL) {
                int value = 1;
                setItemHashTable(bad_table, bad_str, &value, HashString);
            } else {
                (*bad_cnt)++;
            }
        }
        
        printf("\nCollision count (bad cases): %lu", getCollisionCount(bad_table, HashString));
        printf("\nCollision count (fast) (bad cases): %lu\n", getCollisionCountFast(bad_table));
        printf("\nCapacity: %d\n", (int)bad_table->capacity);
        
        fclose(bad_file);
        freeHashTable(bad_table);
    }
    
    printf("\n\u001B[32mAll tests passed!\u001B[0m\n");
    return 0;
}