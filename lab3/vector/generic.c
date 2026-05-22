#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "generic.h"


const size_t INCREASE_FACTOR = 2;
const size_t INIT_CAPACITY = 10;
const size_t MIN_CAPACITY = 4;


// Вспомогательная функция для изменения размера
static bool needToResize(Vector *vector, bool *increase)
{
    if (vector->capacity <= vector->size) {
        *increase = true;
        return true;
    }

    if (vector->size < vector->capacity / (2 * INCREASE_FACTOR)) {
        *increase = false;
        return true;
    }

    return false;
}


// Определяем увеличивать размер или уменьшать
static int resize(Vector *vector, bool increase)
{
    size_t new_capacity;

    // Увеличиваем размер capacity
    if (increase) {
        new_capacity = vector->capacity * INCREASE_FACTOR;
        if (new_capacity < vector->capacity) {
            return -1;  // overflow
        }
    }

    // Уменьшаем размер capacity
    else {
        new_capacity = vector->capacity / INCREASE_FACTOR;

        // Если capacity меньше, чем минимальный
        if (new_capacity < MIN_CAPACITY) {
            new_capacity = MIN_CAPACITY;
        }

        // Не уменьшаем, если не хватит места
        if (vector->size > new_capacity) {
            return 0;
        }
    }

    // Изменяем размер vector
    void* new_data = realloc(vector->data, new_capacity * vector->elem_size);
    
    // Проверка на выделение памяти
    if (new_data == NULL) {
        return -1;
    }
    
    // Присваивание новых полей вектору
    vector->data = new_data;
    vector->capacity = new_capacity;
    
    // Успех
    return 0;
}


Vector *createVector(size_t elem_size)
{
    if (elem_size == 0) {
        return NULL;
    }

    // Выделяем память
    Vector* vector = malloc(sizeof(Vector));

    // Проверяем, удалось ли выделить память
    if (vector == NULL) {
        return vector;
    }

    // Выделяем память под data
    vector->data = malloc(elem_size * INIT_CAPACITY);
    if (vector->data == NULL) {
        free(vector);
        return NULL;
    }

    // Задаём начальную вместимость и размер элемента
    vector->size = 0;
    vector->capacity = INIT_CAPACITY;
    vector->elem_size = elem_size;

    return vector;
}


int appendVectorItem(Vector *vector, void *el)
{
    if (vector == NULL || el == NULL || vector->data == NULL) {
        return -1;
    }
    
    bool increase;
    if (needToResize(vector, &increase)) {
        int resize_result = resize(vector, increase);

        if (resize_result == -1) {
            return -1;
        }
    }

    // Предположим, что со вместимостью всё в порядке и место ещё есть
    memcpy((char*)vector->data + (vector->size * vector->elem_size), el, vector->elem_size);
    vector->size++;
    
    // Успех
    return 0;
}


void *getVectorItem(Vector *vector, size_t index)
{
    if (vector == NULL || vector->data == NULL || vector->size <= index) {
        return NULL;
    }
    
    void* index_pointer = (char*)vector->data + (vector->elem_size * index);
    return index_pointer;
}


int setVectorItem(Vector *vector, size_t index, void *value)
{
    if (vector == NULL || vector->data == NULL || vector->size <= index || value == NULL) {
        return -1;
    }

    void* index_pointer = (char*)vector->data + (vector->elem_size * index);
    memcpy(index_pointer, value, vector->elem_size);

    return 0;
}


void *popVectorItem(Vector *vector, size_t index)
{
    if (vector == NULL || vector->data == NULL || vector->size <= index) {
        return NULL;
    }
    
    // Получаем указатель на удаляемый элемент
    void* index_pointer = (char*)vector->data + (vector->elem_size * index);

    // Копируем удаляемые данные и возвращаем
    void* data_copy = malloc(vector->elem_size);
    if (data_copy == NULL) {
        return NULL;
    }

    memcpy(data_copy, index_pointer, vector->elem_size);

    // Удаляем элемент. Перекопируем блок
    if (index < vector->size - 1)  // Если удаляем не последний элемент
    {
        void* ptr_dst = (char*)vector->data + (vector->elem_size * index);
        void* ptr_src = (char*)vector->data + (vector->elem_size * (index + 1));
        size_t bytes_count = vector->elem_size * (vector->size - (index + 1));
        memmove(ptr_dst, ptr_src, bytes_count);
    }

    // Уменьшаем размер вектора. Хвост просто не будет читаться
    vector->size--;

    // Проверяем, нужно ли изменение размера, и если да - меняем
    bool increase;
    if (needToResize(vector, &increase)) {
        int resize_result = resize(vector, increase);

        if (resize_result == -1) {
            free(data_copy);
            return NULL;
        }
    }

    return data_copy;
}


long int findVectorItem(Vector *vector, void *value, EqualsFunc cmp)
{
    if (vector == NULL || vector->data == NULL || value == NULL) {
        return -2;
    }

    // Если функции cmp нет 
    if (cmp == NULL) {
        for (size_t index = 0; index < vector->size; index++) {
            void* cmp_data = (char*)vector->data + (vector->elem_size * index);
            if (memcmp(value, cmp_data, vector->elem_size) == 0) {
                long int return_index = index;
                return return_index;
            }
        }
    }

    else {
        for (size_t index = 0; index < vector->size; index++) {
            void* cmp_data = (char*)vector->data + (vector->elem_size * index);
            if (cmp(value, cmp_data)) {
                long int return_index = index;
                return return_index;
            }
        }
    }

    return -1;
}

// NEW: Функция сортировки вектора
// cmp — это стандартный компаратор: int (*)(const void *, const void *)
void vectorSort(Vector *vector, int (*cmp)(const void *, const void *)) {
    if (vector == NULL || vector->data == NULL || vector->size < 2 || cmp == NULL) {
        return;
    }

    // Используем стандартный qsort
    // vector->data — указатель на начало массива
    // vector->size — количество элементов
    // vector->elem_size — размер одного элемента в байтах
    // cmp — функция сравнения
    qsort(vector->data, vector->size, vector->elem_size, cmp);
}


int vectorFree(Vector *vector)
{
    if (vector == NULL) {
        return -1;
    }

    free(vector->data);
    free(vector);

    return 0;
}