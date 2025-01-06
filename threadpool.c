#include <malloc.h>
#include <stdlib.h>
#include "threadpool.h"
// Enqueue function: Add a task to the end of the queue
void enqueue(work_t** qhead, work_t** qtail, int* qsize, int (*routine)(void*), void* arg) {
    // Create a new work item
    work_t* new_work = (work_t*)malloc(sizeof(work_t));
    if (new_work == NULL) {
        exit(EXIT_FAILURE);
    }
    new_work->routine = routine;
    new_work->arg = arg;
    new_work->next = NULL;

    if (*qtail == NULL) {
        // If the queue is empty, both head and tail point to the new work
        *qhead = *qtail = new_work;
    } else {
        // Add the new work to the end of the queue and update the tail
        (*qtail)->next = new_work;
        *qtail = new_work;
    }
    (*qsize) ++;
}

// Dequeue function: Remove and return a task from the front of the queue
work_t* dequeue(work_t** qhead, work_t** qtail, int* qsize) {
    if (*qhead == NULL) {
        // Queue is empty
        return NULL;
    }

    work_t* task = *qhead;  // Get the task at the front of the queue
    *qhead = (*qhead)->next;

    if (*qhead == NULL) {
        // If the queue becomes empty, set the tail to NULL
        *qtail = NULL;
    }
    (*qsize) --;

    return task;  // Return the dequeued task
}

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
    pthread_mutex_lock(&from_me->qlock);


    if (from_me->dont_accept) {
        pthread_mutex_unlock(&from_me->qlock);
        return;
    }

    while (from_me->qsize == from_me->max_qsize) {
        pthread_cond_wait(&from_me->q_not_full, &from_me->qlock);
    }

    enqueue(&from_me->qhead, &from_me->qtail, &from_me->qsize, *dispatch_to_here, arg);


    pthread_cond_signal(&from_me->q_not_empty);
    pthread_mutex_unlock(&from_me->qlock);
}

void* do_work(void* p) {
    threadpool* tp = (threadpool*)p;
    while (1) {
        pthread_mutex_lock(&tp->qlock);

        if (tp->shutdown) {
            pthread_mutex_unlock(&tp->qlock);
            pthread_exit(NULL);
        }

        if (tp->qsize == 0) {
            pthread_cond_wait(&tp->q_not_empty, &tp->qlock);
        }
        if (tp->shutdown) {
            pthread_mutex_unlock(&tp->qlock);
            pthread_exit(NULL);
        }

        work_t* work = dequeue(&tp->qhead, &tp->qtail, &tp->qsize);

        if (tp->qsize == tp->max_qsize - 1){
            pthread_cond_signal(&tp->q_not_full);
        }

        if(tp->qsize == 0 && tp->dont_accept) {
            pthread_cond_signal(&tp->q_empty);
        }

        pthread_mutex_unlock(&tp->qlock);

        if (work != NULL) {
            work->routine(work->arg);
            free(work);
        }
    }
}

void destroy_threadpool(threadpool* destroyme) {
    if (destroyme == NULL) return;

    pthread_mutex_lock(&destroyme->qlock);

    destroyme->dont_accept = 1;

    while (destroyme->qsize > 0) {
        pthread_cond_wait(&destroyme->q_empty, &destroyme->qlock);
    }

    destroyme->shutdown = 1;
    pthread_cond_broadcast(&destroyme->q_not_empty);
    pthread_mutex_unlock(&destroyme->qlock);

    for (int i = 0; i < destroyme->num_threads; i++) {
        pthread_join(destroyme->threads[i], NULL);
    }

    work_t* current = destroyme->qhead;
    while (current != NULL) {
        work_t* temp = current;
        current = current->next;
        free(temp);
    }

    pthread_mutex_destroy(&destroyme->qlock);
    pthread_cond_destroy(&destroyme->q_not_empty);
    pthread_cond_destroy(&destroyme->q_empty);
    pthread_cond_destroy(&destroyme->q_not_full);
    free(destroyme->threads);
    free(destroyme);
}

