/*
 Preservation Virtual Machine Project

 Yet another ivm emulator

 Authors:
    Sergio Romero Montiel
    Eladio Gutierrez Carrasco
    Oscar Plata Gonzalez

 Date: May 2020

 Thread-based parallel output header file
*/

#ifndef __IO_HANDLER_H__
#define __IO_HANDLER_H__
#include <pthread.h>
#include "list.h"

// element lists handler to feed worker threads
struct QueueHandler{
    LinkedQueue_t freeQueue;
    LinkedQueue_t waitQueue;
    long requested;
    long processed;
};
typedef struct QueueHandler* QueueHandler_t;
extern QueueHandler_t queueHandler;

typedef void (*Funct_t)(void*);
typedef void* (*VoidFunct_t)();

// the interface function to be invoqued to make parallel output
extern void ioInitParallel(VoidFunct_t newElem, Funct_t flush);

//  together with ioFlush() macro
#define ioEnqueue(publicElem)                               \
    do{ enqueueWait(queueHandler,publicElem);               \
        __sync_fetch_and_add(&queueHandler->requested, 1);  \
        publicElem = getFreeElem(queueHandler);             \
    }while(0)

extern QueueHandler_t newQueueHandler();
extern void* getFreeElem(QueueHandler_t qH);
extern int waitUntilProcessed(QueueHandler_t qH);

inline int enqueueFree(QueueHandler_t qH, void* elem)
{ return (qH ? enqueue(qH->freeQueue, elem) : -1); }

inline int enqueueWait(QueueHandler_t qH, void* elem)
{ return (qH ? enqueue(qH->waitQueue, elem) : -1); }

inline void* dequeueFree(QueueHandler_t qH)
{ return (qH ? dequeue(qH->freeQueue,1) : NULL); }

inline void* dequeueWait(QueueHandler_t qH)
{ return (qH ? dequeue(qH->waitQueue,1) : NULL); }

#endif



