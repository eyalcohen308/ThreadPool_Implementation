#include <stdio.h>
#include <pthread.h>

#include "threadPool.h"

#define  ERROR_PRINT "Error in system call\n"

void writeError(ThreadPool *threadpool) {
    write(2, ERROR_PRINT, strlen(ERROR_PRINT));
    // free all the memory that allocated
    //TODO: create free threads function.
    // freeAllThreadPool(threadpool);
    exit(-1);
}

/**
 * Create threads array size num of threads.
 * @param tp tread pool ref.
 * @param numOfThreads to create.
 */
void initThreadsArray(ThreadPool *tp, int numOfThreads) {
    int i = 0;
    tp->threadsArray = (pthread_t *) malloc(sizeof(pthread_t) * numOfThreads);
    tp->threadsArray == NULL ? writeError(tp) : NULL;
    for (; i < numOfThreads; ++i) {
        // TODO: create threadFunc function which handle with tasks.
        // pthread_create(&(tp->threadsArray[i]), NULL, threadFunc, (void *) tp)!=0? writeError(tp) : NULL;
    }
}

/**
 * Create thread pool with numOfThreads threads.
 * @param numOfThreads number of threads in the pool.
 * @return pointer to the tp object.
 */
ThreadPool *tpCreate(int numOfThreads) {
    // first, allocate space for the tp.
    ThreadPool *tp = (ThreadPool *) malloc(sizeof(ThreadPool));

    // check if succeed, and return null if not.
    if (tp == NULL) {
        writeError(tp);
        return NULL;
    }

    // init all tp data.
    tp->activateStatus = NORMAL;
    tp->osQueue = osCreateQueue();
    tp->numOfThreads = numOfThreads;
    // create mutex and pthread cond, and end program if failed.
    pthread_mutex_init(&tp->lock, NULL) != 0 ? writeError(tp) : NULL;
    pthread_cond_init(&tp->cond, NULL) != 0 ? writeError(tp) : NULL;

    initThreadsArray(tp, numOfThreads);
    return tp;
}

/**
 * Destroy the thread pool and release all alocated memory.
 * also get int shouldWaitForTasks. if it's *not* zero the tread pool will finish
 * all the task that already running and also those in the queue.
 * otherwise, finish only the running tasks and not those in the queue.
 * @param threadPool to destroy.
 * @param shouldWaitForTasks
 */
void tpDestroy(ThreadPool *threadPool, int shouldWaitForTasks) {

}