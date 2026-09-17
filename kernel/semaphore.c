#include "../include/thread.h"
#include "../include/process.h"

extern pcb_t process_table[MAX_PROCESSES];
extern int current_process;

void sem_init(semaphore_t *s, int initial_count) {
    s->count = initial_count;
    s->wait_head = 0;
    s->wait_tail = 0;
}

void sem_wait(semaphore_t *s) {
    __asm__ __volatile__("cli");
    if (s->count > 0) {
        s->count--;
        __asm__ __volatile__("sti");
    } else {
        if (s->wait_tail < 64) {
            s->waiting_queue[s->wait_tail++] = current_process;
            process_table[current_process].state = PROC_STATE_BLOCKED;
        }
        __asm__ __volatile__("sti");
        scheduler_yield();
    }
}

void sem_signal(semaphore_t *s) {
    __asm__ __volatile__("cli");
    if (s->wait_head < s->wait_tail) {
        int next_pid = s->waiting_queue[s->wait_head++];
        process_table[next_pid].state = PROC_STATE_READY;
    } else {
        s->count++;
    }
    __asm__ __volatile__("sti");
}
