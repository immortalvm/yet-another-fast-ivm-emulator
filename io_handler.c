/*
 Preservation Virtual Machine Project

 Yet another ivm emulator

 Authors:
    Sergio Romero Montiel
    Eladio Gutierrez Carrasco
    Oscar Plata Gonzalez

 Date: May 2020

 Thread-based parallel output implementation file
*/


#include <stdlib.h>
#include "io_handler.h"

QueueHandler_t queueHandler;


static QueueHandler_t initQueueHandler( QueueHandler_t qH,
                                        LinkedQueue_t freeQueue,
                                        LinkedQueue_t waitQueue,
                                        VoidFunct_t newElem,
                                        Funct_t process)
{
    if (!qH) return NULL;
    qH->freeQueue = freeQueue;
    qH->waitQueue = waitQueue;
    qH->process = process;
    qH->newElem = newElem;
    return qH;
}

QueueHandler_t newQueueHandler(VoidFunct_t newElem, Funct_t process)
{
    return initQueueHandler((QueueHandler_t)malloc(sizeof(struct QueueHandler)),
                            newQueue(), newQueue(), newElem, process);
}


// dequeue a free elem or create a new one
void* getFreeElem(QueueHandler_t qH)
{
    return (qH ? dequeue(qH->freeQueue,0) ?: qH->newElem() : NULL);
}


// Code for output threads
void* handlerWorker(void *arg)
{
    QueueHandler_t this = (QueueHandler_t)arg;
    Funct_t workRoutine = this->process;
    while(1) {
        void* pElem = dequeueWait(this);
        workRoutine(pElem);
        __sync_fetch_and_add(&this->processed, 1);
        enqueueFree(this,pElem);
    }
}


// The interface functions:
// ioInitParallel to be invoqued to make parallel output
// (together with ioFlush() macro and waitUntilProcessed)
void ioInitParallel(VoidFunct_t newElem, Funct_t flush_r)
{
    queueHandler = newQueueHandler(newElem,flush_r);

    #if !defined(NUM_THREADS)
    // Default number of threads
    int numThreads = 8;    
    #else
    // defined with -DNUM_THREADS=N at compile time
    int numThreads = NUM_THREADS;
    #endif
    // if environment variable NUM_THREADS=N exists, this value is used instead
    //  of any previous value
    char *nthreads_str = getenv("NUM_THREADS");
    if (nthreads_str) numThreads = atoi(nthreads_str);
    // Must be 2 or more: 1 thread for emulation and (N-1) threads for io
    if (numThreads < 2) numThreads = 2;
    #if (VERBOSE>0)
    printf("Number of threads: %d\n",numThreads);
    #endif
    for (int i=0; i<numThreads-1; i++) {
        pthread_t tid;
        pthread_create(&tid, NULL, handlerWorker, queueHandler);
        pthread_detach(tid);
    }
}

// call waitUntilProcessed to ensure all output are processed and every
//  outputElement are freed
int waitUntilProcessed(QueueHandler_t qH)
{
    if (qH == NULL) return -1;
    void *elem;
    while (qH->processed < qH->requested) {
        elem = dequeueFree(qH);
        free(elem);
    }
    return 0;
}

    
