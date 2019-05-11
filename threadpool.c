#include <stdio.h>
#include <pthread.h>

#include "threadPool.h"

#define  ERROR_PRINT "Error in system call\n"

/**
 * free the threadpool, first free the queue and than the pointer itself. no need for release the
 * pthreads because it done by itself.
 * @param threadpool to free.
 */
void freeThreadPool(ThreadPool *threadpool) {

    Task *task;
    while (!osIsQueueEmpty(threadpool->osQueue)) {
        task = osDequeue(threadpool->osQueue);
        if (task != NULL) {
            free(task);
            task = NULL;
        }
    }

    osDestroyQueue(threadpool->osQueue);
    threadpool->osQueue = NULL;

    // free threads array allocation.
    if (threadpool->threadsArray != NULL) {
        free(threadpool->threadsArray);
    }

    // lock the mutex for only one thread will destroy itself and the condition.
    if (pthread_mutex_lock(&threadpool->lock) != 0) {
        write(2, ERROR_PRINT, strlen(ERROR_PRINT));
        free(threadpool);
        exit(-1);
    }

    // destroy the cond and the mutex.
    pthread_mutex_destroy(&threadpool->lock);
    pthread_cond_destroy(&threadpool->cond);
    // mark as finished.
    threadpool->taskStatus = DONE;
    // free the threadpool itself.
    if (threadpool != NULL) free(threadpool);
}

/**
 * write error to the error file.
 * @param threadpool thread pool for killing all the threads.
 */
void writeError(ThreadPool *threadpool) {
    write(2, ERROR_PRINT, strlen(ERROR_PRINT));
    // free all the memory that allocated
    freeThreadPool(threadpool);
    exit(-1);
}

/**
 * the thread function task that some thread will run.
 * @param func threadpool.
 * @return pointer.
 */
void *threadFunction(void *func) {
    // downcast to threadpool:
    ThreadPool *threadpool = (ThreadPool *) func;
    Task *task;
    while (threadpool->taskStatus != DONE) {
        // unlock the mutex if locked and write error if there is problem.
        if (pthread_mutex_unlock(&threadpool->lock) != 0) writeError(threadpool);
        /**
         * end the while loop if the queue is empty with wait status or
         *  on DO_NOT_WAIT mode when no need to activate the tasks in the queue.
         */
        if ((osIsQueueEmpty(threadpool->osQueue) && threadpool->taskStatus == WAIT) ||
            threadpool->taskStatus == DO_NOT_WAIT) {
            if (pthread_mutex_unlock(&threadpool->lock) != 0) writeError(threadpool);
            break;
        }
        // synronize the task running, so lock the next section:
        if (pthread_mutex_lock(&threadpool->lock) != 0) writeError(threadpool);

        // waiting satus: if the queue is empty, will wait for tasks to be enqueued.
        while (threadpool->taskStatus == NORMAL && osIsQueueEmpty(threadpool->osQueue)) {
            pthread_cond_wait(&threadpool->cond, &threadpool->lock);
        }

        // if the queue is empty - unlock and continue
        if (osIsQueueEmpty(threadpool->osQueue)) {
            if (pthread_mutex_unlock(&threadpool->lock) != 0) writeError(threadpool);
            continue;
        }

        //if there are task to run, get task and if its not null - unlock, run and free the task!
        task = osDequeue(threadpool->osQueue);
        if (task != NULL) {
            if (pthread_mutex_unlock(&threadpool->lock) != 0) writeError(threadpool);
            ((task->computeFunc))(task->param);
            free(task);
        }
    }
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
        pthread_create(&(tp->threadsArray[i]), NULL, threadFunction, (void *) tp) != 0 ? writeError(tp) : NULL;
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
    tp->taskStatus = NORMAL;
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
 * also get int shouldWaitForTasks. if it's *not* 0 the tread pool will finish
 * all the task that already running and also those in the queue.
 * otherwise, finish only the running tasks and not those in the queue.
 * @param threadPool to destroy.
 * @param shouldWaitForTasks
 */
void tpDestroy(ThreadPool *threadPool, int shouldWaitForTasks) {
    int i = 0;
    int size = threadPool->numOfThreads;
    if (threadPool->taskStatus != NORMAL) {
        if (pthread_mutex_unlock(&threadPool->lock) != 0) writeError(threadPool);
        return;
    }
    threadPool->taskStatus = ((shouldWaitForTasks == 0) ? DO_NOT_WAIT : WAIT);
    // wake up all threads.
    pthread_cond_broadcast(&threadPool->cond);
    //wait for all threads to finish their jobs.
    for (; i < size; ++i) {
        pthread_join(threadPool->threadsArray[i], NULL);
    }
    freeThreadPool(threadPool);
    threadPool = NULL;
}

/**
 * Insert task to the thread pool, insert it to the task queue.
 * @param threadPool to insert to.
 * @param computeFunc the func needed to be done.
 * @param param parameters for the func.
 * @return 0 if succeed -1 otherwise.
 */
int tpInsertTask(ThreadPool *threadPool, void (*computeFunc)(void *), void *param) {
    if (threadPool->taskStatus != NORMAL) return -1;
    else {
        // lock the mutex, if failed write error.
        if (pthread_mutex_lock(&threadPool->lock) != 0) {
            writeError(threadPool);
        }
        Task *task = (Task *) malloc(sizeof(Task));
        // if allocation failed write error and return -1.
        if (task == NULL) {
            writeError(threadPool);
            return -1;
        }

        //init task data:
        task->computeFunc = computeFunc;
        task->param = param;

        // enqueue task to the treadpool:
        osEnqueue(threadPool->osQueue, task);
        // wake up one thread if exists one who sleep.
        pthread_cond_signal(&threadPool->cond);

        // unlock the mutex for regular fluent.
        pthread_mutex_unlock(&threadPool->lock);
        return 0;
    }
}