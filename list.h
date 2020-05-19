/*
 Preservation Virtual Machine Project

 Yet another ivm emulator

 Authors:
    Sergio Romero Montiel
    Eladio Gutierrez Carrasco
    Oscar Plata Gonzalez

 Date: May 2020

 Simple linked list header file
*/
#ifndef __LIST_H__
#define __LIST_H__

typedef struct LinkedQueue* LinkedQueue_t;

extern int initQueue(LinkedQueue_t list);
extern LinkedQueue_t newQueue();
extern int enqueue(LinkedQueue_t list, void* elem);
extern void* dequeue(LinkedQueue_t list, int wait);

#endif
