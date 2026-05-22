#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "../vector/generic.h"
#include "generic.h"

// 3 вариант: Свободная адресация (линейное исследование)   Метод деления



// Дополнительная функция для вектора
Vector *createVectorBySize(size_t init_capacity, size_t elem_size)
{
    if (elem_size == 0) {
        return NULL;
    }

    // Выделяем память
    Vector* vector = malloc(sizeof(Vector));

    // Проверяем, удалось ли выделить память
    if (vector == NULL) {
        return NULL;
    }

    // Выделяем память под data
    vector->data = calloc(init_capacity, elem_size);
    if (vector->data == NULL) {
        free(vector);
        return NULL;
    }

    // Задаём начальную вместимость и размер элемента
    vector->size = init_capacity;
    vector->capacity = init_capacity;
    vector->elem_size = elem_size;

    return vector;
}




// Во сколько раз увеличивать таблицу
const size_t TABLE_INCREASE_FACTOR = 2;

// // Метод деления
// int HashInt(const void *key)
// {
//     // key - число. Оно по умолчанию уникально.
//     return *(int*)key; // % table->capacity - не забыть % после return
// }

int HashLong(const void *key) {
    // Читаем все 8 байт!
    unsigned long long val = *(unsigned long long*)key;
    // Смешиваем верхнюю и нижнюю части, чтобы хэш был качественным
    return (int)(val ^ (val >> 32));
}

// Метод деления
// Алгоритм на C-строках djb2
int HashString(const void *key)
{
    // Начальное простое число
    int hash = 5381;
    int c;

    // key - строчка

    // Цикл, который:
    //    Приводит key к строке,
    //    Затем берёт один символ
    //    Присваивает его переменной c
    //    Переходит к следующему символу в строке ++
    //    Сам цикл завершится, когда c станет равна '\0' - нулевой байт ~ False
    while ((c = *(char*)key++)) {
        // hash << 5 - побитовый сдвиг влево
        // Равносильно умножени числа на 32 ( 2^5 )
        // Затем добавление hash, что теперь равносильно умножению на 33
        // Добавление c (ASCII код текущего символа) 
        hash = ((hash << 5) + hash) + c; // hash * 33 + c; << - побитовый сдвиг быстрее, чем умножение

        // 33 - так как это эмпирически подобранная константа
    }
    return hash; // % table->capacity - не забыть % после return
}

size_t GenOptArrLen() 
{
    // Для 5 лабораторной
    // (2^k) - 1 < num < 2^k
    // Так как может случиться так, что длинну массива можно представить в виде 2^k или (2^k) -1
    // Но если мы возьмём остаток от деления на 2^k или 2^(k-1) от хэша, тогда
    // возьмутся только последние k байт. И они будут определять хэш. Это не очень хорошо.
    // Поэтому можно взять простые числа. Но не все. Ведь некоторые простые числа можно 
    // представить ввиде (2^k) - 1, например, 7. 2^3 - 1 = 8 - 1 = 7.
    // Тогда мы просто возьмём простые числа из диапазона (2^k) - 1 < num < 2^k

    // Генерация простых чисел до n элементарным перебором O(n^2)

    // Проверка на условие

    // Возвращение массива простых чисел, подходящих под условие

    return 0;
}

HashTable *createHashTable(size_t key_size, size_t val_size)
{
    if (key_size == 0 || val_size == 0) return NULL;

    HashTable* table = malloc(sizeof(HashTable));
    if (table == NULL) return NULL;
    
    table->values = createVectorBySize(TABLE_MIN_SIZE, sizeof(char) + key_size + val_size);
    if (table->values == NULL) {
        free(table);
        return NULL;
    }

    // Заполнение полей таблицы
    table->key_size = key_size;
    table->val_size = val_size;
    table->size = 0;
    table->collision_count = 0;
    table->capacity = TABLE_MIN_SIZE;

    return table;
}

void setItemHashTable(HashTable *table, void *key, void *data, HashFunc hash) {
    if (table == NULL || key == NULL || data == NULL || hash == NULL) {
        return;
    }

    // Преобразование хэша в индекс массива (key = index, value = data)
    const size_t index = hash(key) % table->capacity;

    // Пара - массив из 3-х элементов. Первый - флаг. Второй и третий - ключ и значение.
    void* pair = malloc(sizeof(char) + table->key_size + table->val_size);
    if (pair == NULL) return;

    // Заполняем пару данными
    char flag = SLOT_OCCUPIED;
    memcpy(pair, &flag, sizeof(char));
    memcpy((char*)pair + sizeof(char), key, table->key_size);
    memcpy((char*)pair + sizeof(char) + table->key_size, data, table->val_size);

    size_t counter = 0;
    while (counter <= table->capacity) {
        // Квадратичная индексация для 5 лабы
        // size_t index_new = (index + (counter * counter)) % table->capacity;

        // Линейное исследование
        size_t index_new = (index + counter) % table->capacity;

        void* item = getVectorItem(table->values, index_new);

        // Если слот пустой или ключ пользователя равен ключу в таблице, нужно вставить или перезаписать значение 
        if (*(char*)item == SLOT_EMPTY || memcmp(key, (char*)item + sizeof(char), table->key_size) == 0) {
            int inserted = setVectorItem(table->values, index_new, pair);
            if (inserted == -1) return;
            free(pair);

            if (index != index_new) table->collision_count++;
            // if (*(char*)item == SLOT_EMPTY) table->size++;
            table->size++;

            break;
        }
        counter++;
    }

    // Перевод в проценты
    if ((table->size * 100 / table->capacity) > MAX_FILL_PERCENT) {
        rehashHashTable(table, hash);
    }
}

void rehashHashTable(HashTable *table, HashFunc hash)
{
    Vector* pairs = table->values;

    // Увеличиваем вместимость таблицы в два раза
    Vector* new_pairs = createVectorBySize(table->capacity * TABLE_INCREASE_FACTOR, sizeof(char) + table->key_size + table->val_size);
    if (new_pairs == NULL) return;

    table->values = new_pairs;
    table->size = 0;
    table->collision_count = 0;
    table->capacity *= TABLE_INCREASE_FACTOR;

    for (size_t i = 0; i < pairs->capacity; i++) {
        void* pair = getVectorItem(pairs, i);

        // Копируем не все элементы, а только занятые
        if (*(char*)pair == SLOT_OCCUPIED) {
            void* key = pair + sizeof(char);
            void* data = pair + sizeof(char) + table->key_size;

            // Флаг выставляется автоматически, коллизии пересчитываются
            setItemHashTable(table, key, data, hash);
        }
    }

    vectorFree(pairs);
}

void *getItemHashTable(HashTable *table, void *key, HashFunc hash) {
    if (table == NULL || key == NULL || hash == NULL) {
        return NULL;
    }

    const size_t index = hash(key) % table->capacity;
    void* value = NULL;

    int counter = 0;
    while (value == NULL) {
        // Квадратичное пробирование для 5 лабы
        // int index_new = (index + (counter * counter)) % table->capacity;
        // Линейное исследование
        size_t index_new = (index + counter) % table->capacity;

        void* item = getVectorItem(table->values, index_new);
        if (item == NULL) return NULL;

        if (*(char*)item == SLOT_EMPTY) return NULL;
        if (*(char*)item == SLOT_DELETED) return NULL;

        // Если ключ, который ввёл пользователь и ключ из таблицы совпадают
        // То нужно взять это значение. Теперь оно != NULL и цикл завершится
        if (memcmp(key, item + sizeof(char), table->key_size) == 0) {
            value = item + sizeof(char) + table->key_size;
        }

        counter++;
    }

    return value;
}

// NEW: Функция, которая печатает все ключи из таблицы
void print_all_keys(HashTable* table) {
    if (table == NULL || table->values == NULL) return;

    for (size_t i = 0; i < table->capacity; i++) {
        // Получаем указатель на текущую ячейку
        void* item = getVectorItem(table->values, i);

        if (item == NULL) continue;

        // Проверяем статус слота
        char status = *(char*)item;

        // Ключ интересует только если слот ЗАНЯТ (OCCUPIED)
        if (status == SLOT_OCCUPIED) { 
            // Вычисляем адрес ключа (пропускаем 1 байт статуса)
            void* current_key = (char*)item + sizeof(char);
    
            printf("Index [%zu]: %d\n", i, *(int*)current_key);
            printf("Value [%zu]: %d\n", i, *(int*)getItemHashTable(table, current_key, HashLong));
        }
    }
}

void *popItemHashTable(HashTable *table, void *key, HashFunc hash) {
    if (table == NULL || table-> values == NULL || key == NULL || hash == NULL) return NULL;

    Vector* pairs = table->values;
    size_t pop_index = hash(key) % pairs->capacity;

    void* pair = getVectorItem(pairs, pop_index);
    if (*(int*)pair == SLOT_EMPTY) return NULL;  // Если пару вообще не удалось получить

    void* value_return = malloc(table->val_size);
    if (value_return == NULL) return NULL;

    memcpy(value_return, pair + sizeof(char) + table->key_size, table->val_size);

    *(char*)pair = SLOT_DELETED;

    table->size--;

    return value_return;
}

unsigned long int getCollisionCount(HashTable *table, HashFunc hash)
{
    if (table == NULL || table->values == NULL || hash == NULL) {
        return 0;
    }

    // Посчитать хэш от ключа, сверить с индексом. Если индексы равны, то нет коллизии
    // Иначе cnt++

    Vector* pairs = table->values;

    unsigned long int counter = 0;
    for (int i = 0; i < pairs->size; i++) {
        // Получаем пару (флаг, ключ, значение)
        void* pair = getVectorItem(pairs, i);

        // Пропускаем пустые и удалённые элементы
        if (*(char*)pair == SLOT_EMPTY || *(char*)pair == SLOT_DELETED) continue;

        void* key = pair + sizeof(char);
        int key_index = hash(key) % table->capacity;
        if (key_index != i) {
            counter++;
        }
    }
    
    return counter;
}

unsigned long int getCollisionCountFast(HashTable *table)
{
    // Вернуть поле collision_count
    if (table == NULL || table->values == NULL) return 0;
    return table->collision_count;
}

void freeHashTable(HashTable *table)
{
    if (table == NULL) return;

    vectorFree(table->values);
    free(table);
}

// NEW: Функция, которая очищает не только всю таблицу,
// но и воспринимает значения как указатели и очищает память по ним с помощью free()
void deepFreeHashTable(HashTable* table) {
    if (table == NULL) return;

    for (size_t i = 0; i < table->capacity; i++) {
        void* item = getVectorItem(table->values, i);
        
        if (item != NULL && *(char*)item == SLOT_OCCUPIED) {
            // Вычисляем адрес, где лежит указатель-значение
            // Смещение: Статус (1) + Размер Ключа
            void* value_ptr_location = (char*)item + sizeof(char) + table->key_size;

            // Достаем сам указатель из этой памяти
            // Мы приводим к void**, так как в таблице лежит именно адрес
            void* actual_data = *(void**)value_ptr_location;

            // Освобождаем память, которую выделил пользователь
            if (actual_data != NULL) {
                free(actual_data);
            }
        }
    }

    // Теперь, когда все "внутренности" чисты, удаляем саму структуру
    freeHashTable(table);
}