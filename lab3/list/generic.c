#include "generic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../comparators.h"


GenericList* createList(size_t elem_size)
{
    // При appendItem выделяется память через malloc
    // malloc(0) - неопределённое поведение
    if (elem_size == 0) {
        return NULL;
    }

    // Выделяем память
    GenericList* list = calloc(1, sizeof(GenericList));

    // Проверяем, удалось ли выделить память
    if (list == NULL) {
        // Завершает программу с сообщением
        printf("\nMemory error!!!\n");
        return NULL;
    }

    // Задаём ссылку на следующий узел и размер элемента
    list -> head = NULL;  // Необязательно
    list -> elem_size = elem_size;

    return list;
}


void appendItem(GenericList *list, void *data) {
    if (list == NULL) {
        return;
    }

    Node* new_node = malloc(sizeof(Node));
    if (new_node == NULL) {
        return;
    }

    new_node -> data = malloc(list -> elem_size);
    if (new_node -> data == NULL) {
        free(new_node);
        return;
    }

    memcpy(new_node -> data, data, list -> elem_size);
    new_node -> next = NULL;

    if (list -> head == NULL) {
        list -> head = new_node;
    }
    else {
        Node* node = list -> head;
        while (node -> next != NULL) {
            node = node -> next;
        }
        node -> next = new_node;
    }
}


int findItem(GenericList *list, void *value, EqualsFunc cmp)
{
    if (list == NULL || cmp == NULL) {
        return -1;
    }

    Node* node = list -> head;
    size_t index = 0;

    while (node != NULL)
    {
        // Любое ненулевое значение = True
        if (cmp(value, node -> data)) {
            return index;
        }

        node = node -> next;
        index++;
    }

    return -1;
}


void *popItem(GenericList *list, size_t index)
{
    // Проверка на нулевой указатель и лист без значений
    if (list == NULL || list -> head == NULL) {
        return NULL;
    }

    Node* node = list -> head;

    // Удаление head
    if (index == 0) {
        void* data_copy = node -> data;
        list -> head = node -> next;
        // node -> data = NULL;
        free(node);
        return data_copy;
    }
    
    // Доходим в цикле до удаляемого элемента -1
    size_t cnt = 0;
    while (node != NULL && cnt < index - 1)
    {
        node = node -> next;
        cnt++;
    }
    
    // Если index больше, чем элементов в массиве, нода будет NULL
    // Если удаляемый элемент за пределами массива
    if (node == NULL || node -> next == NULL) {
        return NULL;
    }

    Node* pop_node = node -> next;
    
    // Обновляем связь между нодами
    node -> next = pop_node -> next;
    
    // Копируем данные, освобождаем память и возвращаем data
    void* data_copy = pop_node -> data;

    free(pop_node);

    return data_copy;
}


void freeList(GenericList *list)
{
    if (list == NULL) {
        return;
    }

    Node* node = list -> head;
    while (node != NULL)
    {
        Node* next_node = node -> next;

        free(node -> data);
        free(node);

        node = next_node;
    }

    free(list);
}


unsigned int listLength(GenericList *list)
{
    unsigned int lst_elem_cnt = 0;

    if (list == NULL) {
        return lst_elem_cnt;
    }

    Node* node = list -> head;
    while (node != NULL)
    {
        lst_elem_cnt++;
        node = node -> next;
    };

    return lst_elem_cnt;
}