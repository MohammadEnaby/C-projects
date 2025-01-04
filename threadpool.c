#include <malloc.h>
#include "threadpool.h"


threadpool* create_threadpool(int num_threads_in_pool, int max_queue_size) {
    if (max_queue_size <= 0 || num_threads_in_pool <= 0 ||
        num_threads_in_pool > MAXT_IN_POOL || max_queue_size > MAXW_IN_QUEUE) {
        return NULL;
    }

    threadpool* thread_pool = malloc(sizeof(threadpool));
    if (thread_pool == NULL) {
        return NULL;
    }

    thread_pool->num_threads = num_threads_in_pool;
    thread_pool->max_qsize = max_queue_size;
    thread_pool->dont_accept = 0;
    thread_pool->shutdown = 0;
    thread_pool->qsize = 0;
    thread_pool->qhead = NULL;
    thread_pool->qtail = NULL;

    pthread_mutex_init(&thread_pool->qlock, NULL);
    pthread_cond_init(&thread_pool->q_not_empty, NULL);
    pthread_cond_init(&thread_pool->q_empty, NULL);
    pthread_cond_init(&thread_pool->q_not_full, NULL);

    thread_pool->threads = malloc(num_threads_in_pool * sizeof(pthread_t));
    if (thread_pool->threads == NULL) {
        pthread_mutex_destroy(&thread_pool->qlock);
        pthread_cond_destroy(&thread_pool->q_not_empty);
        pthread_cond_destroy(&thread_pool->q_empty);
        pthread_cond_destroy(&thread_pool->q_not_full);
        free(thread_pool);
        return NULL;
    }

    for (int i = 0; i < num_threads_in_pool; i++) {
        if (pthread_create(&thread_pool->threads[i], NULL, do_work, thread_pool) != 0) {
            // Clean up threads created so far
            for (int j = 0; j < i; j++) {
                pthread_cancel(thread_pool->threads[j]);
                pthread_join(thread_pool->threads[j], NULL);
            }

            free(thread_pool->threads);
            pthread_mutex_destroy(&thread_pool->qlock);
            pthread_cond_destroy(&thread_pool->q_not_empty);
            pthread_cond_destroy(&thread_pool->q_empty);
            pthread_cond_destroy(&thread_pool->q_not_full);
            free(thread_pool);
            return NULL;
        }
    }

    return thread_pool;
}

void dispatch(threadpool* from_me, dispatch_fn dispatch_to_here, void* arg) {
    // Lock the mutex to ensure thread safety
    pthread_mutex_lock(&from_me->qlock);

    // Check if the threadpool is no longer accepting jobs
    if (from_me->dont_accept) {
        pthread_mutex_unlock(&from_me->qlock);
        return;
    }

    // Wait if the queue is full
    while (from_me->qsize == from_me->max_qsize) {
        pthread_cond_wait(&from_me->q_not_full, &from_me->qlock);
    }

    // Create and initialize the work_t element
    work_t* work = malloc(sizeof(work_t));
    if (work == NULL) {
        // Handle memory allocation failure
        pthread_mutex_unlock(&from_me->qlock);
        return;
    }
    work->routine = dispatch_to_here;
    work->arg = arg;
    work->next = NULL;

    // Add the work to the queue
    if (from_me->qtail == NULL) {
        from_me->qhead = work;
    } else {
        from_me->qtail->next = work;
    }
    from_me->qtail = work;
    from_me->qsize++;

    // Signal that the queue is not empty
    pthread_cond_signal(&from_me->q_not_empty);

    // Unlock the mutex
    pthread_mutex_unlock(&from_me->qlock);
}



void* do_work(void* p) {
    threadpool* tp = (threadpool*)p;
    while (1) {
        pthread_mutex_lock(&tp->qlock);

        // Wait for work to be available or for shutdown
        while (tp->qsize == 0 && !tp->shutdown) {
            pthread_cond_wait(&tp->q_not_empty, &tp->qlock);
        }

        // If shutdown is set, exit the thread
        if (tp->shutdown) {
            pthread_mutex_unlock(&tp->qlock);
            pthread_exit(NULL);
        }

        // Get the first work item from the queue
        work_t* work = tp->qhead;
        if (work != NULL) {
            tp->qhead = work->next;
            tp->qsize--;

            // Signal if the queue becomes empty
            if (tp->qsize == 0) {
                pthread_cond_signal(&tp->q_empty);
            }

            // If the queue was full, signal that it's no longer full
            if (tp->qsize == tp->max_qsize - 1) {
                pthread_cond_signal(&tp->q_not_full);
            }
        }

        pthread_mutex_unlock(&tp->qlock);

        // Process the work item outside the critical section
        if (work != NULL) {
            work->routine(work->arg);
            free(work); // Free the memory for the work item
        }
    }
}


void destroy_threadpool(threadpool* destroyme) {
    if (destroyme == NULL) return;

    // Step 1: Lock the mutex
    pthread_mutex_lock(&destroyme->qlock);

    // Step 2: Stop accepting new tasks
    destroyme->dont_accept = 1;

    // Step 3: Wait for the queue to be empty
    while (destroyme->qsize > 0) {
        pthread_cond_wait(&destroyme->q_empty, &destroyme->qlock);
    }

    // Step 4: Signal threads to shutdown
    destroyme->shutdown = 1;

    // Step 5: Wake up all waiting threads
    pthread_cond_broadcast(&destroyme->q_not_empty);

    // Step 6: Unlock the mutex
    pthread_mutex_unlock(&destroyme->qlock);

    // Step 7: Join all threads
    for (int i = 0; i < destroyme->num_threads; i++) {
        pthread_join(destroyme->threads[i], NULL);
    }

    // Step 8: Free remaining resources
    work_t* current = destroyme->qhead;
    while (current != NULL) {
        work_t* temp = current;
        current = current->next;
        free(temp);
    }

    // Step 9: Clean up synchronization primitives
    pthread_mutex_destroy(&destroyme->qlock);
    pthread_cond_destroy(&destroyme->q_not_empty);
    pthread_cond_destroy(&destroyme->q_empty);
    pthread_cond_destroy(&destroyme->q_not_full);

    // Step 10: Free the thread array and the threadpool structure
    free(destroyme->threads);
    free(destroyme);
}
