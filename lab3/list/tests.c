#include "generic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "../comparators.h"


void test_createList() {
    GenericList* list = createList(sizeof(int));
    assert(list != NULL);
    assert(list->head == NULL);
    assert(list->elem_size == sizeof(int));
    freeList(list);

    list = createList(0);
    assert(list == NULL);

    list = createList(sizeof(char));
    assert(list != NULL);
    assert(list->elem_size == 1);
    freeList(list);
}

void test_appendItem() {
    GenericList* list = createList(sizeof(int));
    assert(listLength(list) == 0);

    int a = 10;
    appendItem(list, &a);
    assert(listLength(list) == 1);
    assert(*(int*)list->head->data == 10);

    int b = 20;
    appendItem(list, &b);
    assert(listLength(list) == 2);
    assert(*(int*)list->head->next->data == 20);

    int c = 30;
    appendItem(list, &c);
    assert(listLength(list) == 3);
    assert(*(int*)list->head->next->next->data == 30);

    freeList(list);
    appendItem(NULL, &a);
}

void test_findItem() {
    GenericList* list = createList(sizeof(int));

    int a = 10, b = 20, c = 30;
    appendItem(list, &a);
    appendItem(list, &b);
    appendItem(list, &c);

    assert(findItem(list, &a, intEquals) == 0);
    assert(findItem(list, &b, intEquals) == 1);
    assert(findItem(list, &c, intEquals) == 2);

    int d = 999;
    assert(findItem(list, &d, intEquals) == -1);
    assert(findItem(NULL, &a, intEquals) == -1);
    assert(findItem(list, &a, NULL) == -1);

    freeList(list);
}

void test_popItem() {
    GenericList* list = createList(sizeof(int));

    int a = 10, b = 20, c = 30;
    appendItem(list, &a);
    appendItem(list, &b);
    appendItem(list, &c);

    void* data = popItem(list, 0);
    assert(data != NULL);
    assert(*(int*)data == 10);
    assert(listLength(list) == 2);
    free(data);

    data = popItem(list, 0);
    assert(data != NULL);
    assert(*(int*)data == 20);
    assert(listLength(list) == 1);
    free(data);

    data = popItem(list, 0);
    assert(data != NULL);
    assert(*(int*)data == 30);
    assert(listLength(list) == 0);
    free(data);

    assert(popItem(list, 0) == NULL);

    appendItem(list, &a);
    assert(popItem(list, 999) == NULL);
    assert(listLength(list) == 1);

    freeList(list);
    assert(popItem(NULL, 0) == NULL);
}

void test_freeList() {
    GenericList* list = createList(sizeof(int));
    int a = 10, b = 20;
    appendItem(list, &a);
    appendItem(list, &b);
    freeList(list);

    freeList(NULL);

    list = createList(sizeof(double));
    freeList(list);
}

// Стресс-тест 1: 100 000 элементов — добавление + удаление с конца
void stress_test_large_append_pop() {
    const int N = 100000;
    GenericList* list = createList(sizeof(int));
    assert(list != NULL);

    // Добавляем 10 000 элементов
    for (int i = 0; i < N; i++) {
        appendItem(list, &i);
        assert(listLength(list) == (size_t)(i + 1));
    }

    // Удаляем все с конца (последний индекс)
    for (int i = 0; i < N; i++) {
        size_t len = listLength(list);
        if (len == 0) break;
        void* data = popItem(list, len - 1);  // удаляем последний
        assert(data != NULL);
        int val = *(int*)data;
        assert(val == N - 1 - i);  // должны идти в обратном порядке: 9999, 9998, ...
        free(data);
    }

    assert(listLength(list) == 0);
    freeList(list);
}

// Стресс-тест 2: 50 000 элементов — добавление + поиск каждого
void stress_test_large_find() {
    const int N = 50000;
    GenericList* list = createList(sizeof(int));
    assert(list != NULL);

    // Добавляем
    for (int i = 0; i < N; i++) {
        appendItem(list, &i);
    }
    assert(listLength(list) == N);

    // Ищем каждый элемент
    for (int i = 0; i < N; i++) {
        int* p = &i;
        int idx = findItem(list, p, intEquals);
        assert(idx == i);  // должен находиться на своей позиции
    }

    freeList(list);
}

// Стресс-тест 3: 10000 элементов — рандомное удаление
void stress_test_random_pop() {
    const int N = 10000;
    GenericList* list = createList(sizeof(int));
    assert(list != NULL);

    // Добавляем
    for (int i = 0; i < N; i++) {
        appendItem(list, &i);
    }

    srand(time(NULL));  // для псевдорандома

    // Удаляем в случайном порядке
    for (int i = 0; i < N; i++) {
        size_t len = listLength(list);
        if (len == 0) break;

        size_t rand_index = rand() % len;  // случайный индекс
        void* data = popItem(list, rand_index);
        assert(data != NULL);
        free(data);
    }

    assert(listLength(list) == 0);
    freeList(list);
}

// Стресс-тест 4: 10000 элементов разных типов (чередуем int и double)
void stress_test_mixed_types() {
    const int N = 10000;
    // Создаём список для double — больше размер
    GenericList* list = createList(sizeof(double));
    assert(list != NULL);

    for (int i = 0; i < N; i++) {
        if (i % 2 == 0) {
            int val = i * 2;
            // Копируем int в double-слот — просто как байты
            appendItem(list, &val);
        } else {
            double val = i * 3.14;
            appendItem(list, &val);
        }
    }

    assert(listLength(list) == N);

    // Просто удаляем всё — не проверяем значения, только стабильность
    while (listLength(list) > 0) {
        void* data = popItem(list, 0);
        assert(data != NULL);
        free(data);
    }

    freeList(list);
}

// Стресс-тест 5: Максимальная нагрузка — 100 000 элементов, только добавление и free
void stress_test_huge_list() {
    const int N = 100000;
    GenericList* list = createList(sizeof(size_t));
    assert(list != NULL);

    for (size_t i = 0; i < N; i++) {
        appendItem(list, &i);
        if (i % 10000 == 0) {
            // Периодически проверяем длину
            assert(listLength(list) == i + 1);
        }
    }

    assert(listLength(list) == N);
    freeList(list);  // главное — не должно упасть и не должно быть утечек
}

// Главный запуск
int main() {
    printf("Running basic tests...\n");
    test_createList();
    test_appendItem();
    test_findItem();
    test_popItem();
    test_freeList();

    printf("Running stress tests...\n");
    stress_test_large_append_pop();
    stress_test_large_find();
    stress_test_random_pop();
    stress_test_mixed_types();
    stress_test_huge_list();

    printf("All tests passed successfully!\n");
    return 0;
}