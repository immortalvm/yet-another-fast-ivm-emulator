/*
 Preservation Virtual Machine Project

 Yet another ivm emulator

 Authors:
    Sergio Romero Montiel
    Eladio Gutierrez Carrasco
    Oscar Plata Gonzalez

 Date: May 2020

 Simple linked list implementation file
*/

#include <stdlib.h>
#include <pthread.h>
#include "list.h"

// linked list
struct ListElement {
    void *payload;
    struct ListElement *next;
};

typedef struct ListElement* ListElement_t;

struct LinkedQueue {
    ListElement_t first;
    ListElement_t last;
    pthread_mutex_t m;
    pthread_cond_t notEmpty;
};



ListElement_t newElemList(void *payload)
{
    ListElement_t elem = (ListElement_t)malloc(sizeof(struct ListElement));
    elem->payload = payload;
    return elem;
}

void* getPayloadElemList(ListElement_t elem)
{
    if (elem == NULL) return NULL;
    return elem->payload;
}

int initQueue(LinkedQueue_t list)
{
    if (!list) return -1;
    list->first = NULL;
    list->last = NULL;
    pthread_mutex_init(&list->m,NULL);
    pthread_cond_init(&list->notEmpty,NULL);
    return 0;
}

LinkedQueue_t newQueue()
{
    LinkedQueue_t list=(LinkedQueue_t)malloc(sizeof(struct LinkedQueue));
    int error = initQueue(list);
    if (error) {
        free(list);
        return NULL;
    }
    return list;
}

int enqueue(LinkedQueue_t list, void *payload)
{
    if (!list || !payload) return -1;
    ListElement_t elem = newElemList(payload);
    elem->next = NULL;
    pthread_mutex_lock(&list->m);
    if (list->last == NULL) list->first = elem;
    else list->last->next = elem;
    list->last = elem;
    pthread_cond_signal(&list->notEmpty);
    pthread_mutex_unlock(&list->m);
    return 0;
}

// if wait == 0 and empty list, return NULL inmediatelly
// else (wait != 0) and empty list, wait until there is an element to return
void* dequeue(LinkedQueue_t list, int wait)
{
    ListElement_t elem;
    if (!list) return NULL;
    pthread_mutex_lock(&list->m);
    if (wait) {
        while (list->first == NULL) pthread_cond_wait(&list->notEmpty,&list->m);
    } else {
        if (list->first == NULL) {
            pthread_mutex_unlock(&list->m);
            return NULL;
        }
    }
    elem = list->first;
    list->first = elem->next;
    if (list->last == elem) list->last = NULL;
    pthread_mutex_unlock(&list->m);
    void *payload = getPayloadElemList(elem);
    free(elem);
    return payload;
}


