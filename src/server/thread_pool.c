// thread_pool.c - Thread Pool Implementation (Phase 8)

#include "../include/thread_pool.h"
#include "../include/request_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <time.h>


// Global thread pool instance
static ThreadPool *g_pool = NULL;

// Worker thread function
void *worker_thread(void *arg)
{
    ThreadPool *pool = (ThreadPool *)arg;
    
    while (1)
    {
        Job job;
        int got_job = 0;
        
        // Lock mutex to access queue
        pthread_mutex_lock(&pool->job_queue.mutex);
        
        // Wait for job or shutdown signal
        while (pool->job_queue.count == 0 && !pool->job_queue.shutdown)
        {
            pthread_cond_wait(&pool->job_queue.not_empty, &pool->job_queue.mutex);
        }
        
        // Check if we should shutdown
        if (pool->job_queue.shutdown && pool->job_queue.count == 0)
        {
            pthread_mutex_unlock(&pool->job_queue.mutex);
            break;
        }
        
        // Get job from queue
        if (pool->job_queue.count > 0)
        {
            job = pool->job_queue.queue[pool->job_queue.head];
            pool->job_queue.head = (pool->job_queue.head + 1) % MAX_QUEUE_SIZE;
            pool->job_queue.count--;
            got_job = 1;
            
            // Signal that queue is not full anymore
            pthread_cond_signal(&pool->job_queue.not_full);
        }
        
        pthread_mutex_unlock(&pool->job_queue.mutex);
        
        // Process job if we got one
        if (got_job)
        {
            int result = handle_client_request(job.client_fd, &job.request);
            
            if (result == 0)
            {
                // Response sent successfully
                // Keep connection open for more requests - don't close it
                // The main server loop will continue to monitor this socket
            }
            else
            {
                // Error occurred, connection will be handled by main loop
            }
            // Don't close the socket - keep it open for more requests
            // The main server loop will handle connection cleanup when client disconnects
        }
    }
    
    return NULL;
}

// Initialize thread pool
int thread_pool_init(ThreadPool *pool)
{
    if (!pool)
        return -1;
    
    // Initialize job queue
    memset(&pool->job_queue, 0, sizeof(JobQueue));
    pool->job_queue.head = 0;
    pool->job_queue.tail = 0;
    pool->job_queue.count = 0;
    pool->job_queue.shutdown = 0;
    
    if (pthread_mutex_init(&pool->job_queue.mutex, NULL) != 0)
        return -1;
    
    if (pthread_cond_init(&pool->job_queue.not_empty, NULL) != 0)
    {
        pthread_mutex_destroy(&pool->job_queue.mutex);
        return -1;
    }
    
    if (pthread_cond_init(&pool->job_queue.not_full, NULL) != 0)
    {
        pthread_mutex_destroy(&pool->job_queue.mutex);
        pthread_cond_destroy(&pool->job_queue.not_empty);
        return -1;
    }
    
    // Create worker threads
    for (int i = 0; i < NUM_WORKER_THREADS; i++)
    {
        if (pthread_create(&pool->threads[i], NULL, worker_thread, pool) != 0)
        {
            // Cleanup on failure
            pool->job_queue.shutdown = 1;
            pthread_cond_broadcast(&pool->job_queue.not_empty);
            
            for (int j = 0; j < i; j++)
            {
                pthread_join(pool->threads[j], NULL);
            }
            
            pthread_mutex_destroy(&pool->job_queue.mutex);
            pthread_cond_destroy(&pool->job_queue.not_empty);
            pthread_cond_destroy(&pool->job_queue.not_full);
            return -1;
        }
    }
    
    g_pool = pool;
    return 0;
}

// Add job to queue
int thread_pool_add_job(ThreadPool *pool, int client_fd, Message *request)
{
    if (!pool || client_fd < 0 || !request)
        return -1;
    
    pthread_mutex_lock(&pool->job_queue.mutex);
    
    // Wait if queue is full
    while (pool->job_queue.count >= MAX_QUEUE_SIZE && !pool->job_queue.shutdown)
    {
        pthread_cond_wait(&pool->job_queue.not_full, &pool->job_queue.mutex);
    }
    
    // Check if pool is shutting down
    if (pool->job_queue.shutdown)
    {
        pthread_mutex_unlock(&pool->job_queue.mutex);
        return -1;
    }
    
    // Add job to queue
    pool->job_queue.queue[pool->job_queue.tail] = (Job){.client_fd = client_fd, .request = *request};
    pool->job_queue.tail = (pool->job_queue.tail + 1) % MAX_QUEUE_SIZE;
    pool->job_queue.count++;
    
    // Signal that queue is not empty
    pthread_cond_signal(&pool->job_queue.not_empty);
    
    pthread_mutex_unlock(&pool->job_queue.mutex);
    
    return 0;
}

// Shutdown thread pool
void thread_pool_shutdown(ThreadPool *pool)
{
    if (!pool)
        return;
    
    pthread_mutex_lock(&pool->job_queue.mutex);
    pool->job_queue.shutdown = 1;
    pthread_cond_broadcast(&pool->job_queue.not_empty);
    pthread_cond_broadcast(&pool->job_queue.not_full);
    pthread_mutex_unlock(&pool->job_queue.mutex);
    
    // Wait for all threads to finish
    for (int i = 0; i < NUM_WORKER_THREADS; i++)
    {
        pthread_join(pool->threads[i], NULL);
    }
    
    // Cleanup synchronization primitives
    pthread_mutex_destroy(&pool->job_queue.mutex);
    pthread_cond_destroy(&pool->job_queue.not_empty);
    pthread_cond_destroy(&pool->job_queue.not_full);
    
    g_pool = NULL;
}
