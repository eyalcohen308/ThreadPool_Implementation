#ifndef __THREAD_POOL__
#define __THREAD_POOL__
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "osqueue.h"
typedef enum {
    NORMAL, WAIT, DONE,DESTROY
} ACTIVATE_STATUS;


typedef struct thread_pool
{
 int numOfThreads;
 OSQueue *osQueue;
 pthread_t *threadsArray;
 pthread_mutex_t lock;
 pthread_cond_t cond;
 ACTIVATE_STATUS activateStatus;
}ThreadPool;

ThreadPool* tpCreate(int numOfThreads);

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param);

#endif
