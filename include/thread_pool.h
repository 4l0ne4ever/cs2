#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include "types.h"
#include "protocol.h"

#define MAX_QUEUE_SIZE 1000
#define NUM_WORKER_THREADS 8

typedef struct
{
    int client_fd;
    Message request;
} Job;

typedef struct
{
    Job queue[MAX_QUEUE_SIZE];
    int head;
    int tail;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
    int shutdown;
} JobQueue;

typedef struct
{
    pthread_t threads[NUM_WORKER_THREADS];
    JobQueue job_queue;
} ThreadPool;

// Initialize thread pool
int thread_pool_init(ThreadPool *pool);

// Add job to queue
int thread_pool_add_job(ThreadPool *pool, int client_fd, Message *request);

// Worker thread function
void *worker_thread(void *arg);

// Shutdown thread pool
void thread_pool_shutdown(ThreadPool *pool);

#endif // THREAD_POOL_H
