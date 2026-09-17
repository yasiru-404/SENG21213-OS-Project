#ifndef THREAD_H
#define THREAD_H

#include "types.h"

// Threads
int thread_create(void (*fn)(void *), void *arg);

// Mutex
typedef struct {
    int locked;
    int owner_pid;
    // We can keep a simple array of waiting processes, or just wake everyone
    // up when unlocked. For simplicity, we'll use a waiting array.
    int waiting_queue[64];
    int wait_head;
    int wait_tail;
} mutex_t;

void mutex_init(mutex_t *m);
void mutex_lock(mutex_t *m);
void mutex_unlock(mutex_t *m);

// Semaphore
typedef struct {
    int count;
    int waiting_queue[64];
    int wait_head;
    int wait_tail;
} semaphore_t;

void sem_init(semaphore_t *s, int initial_count);
void sem_wait(semaphore_t *s);
void sem_signal(semaphore_t *s);

#endif
