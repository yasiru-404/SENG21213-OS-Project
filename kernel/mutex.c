#include "../include/thread.h"
#include "../include/process.h"

extern pcb_t process_table[MAX_PROCESSES];
extern int current_process;

void mutex_init(mutex_t *m) {
    m->locked = 0;
    m->owner_pid = -1;
    m->wait_head = 0;
    m->wait_tail = 0;
}

void mutex_lock(mutex_t *m) {
    __asm__ __volatile__("cli");
    if (!m->locked) {
        m->locked = 1;
        m->owner_pid = current_process;
        __asm__ __volatile__("sti");
    } else {
        // Add to waiting queue
        if (m->wait_tail < 64) {
            m->waiting_queue[m->wait_tail++] = current_process;
            process_table[current_process].state = PROC_STATE_BLOCKED;
        }
        __asm__ __volatile__("sti");
        scheduler_yield(); // Yield the CPU to another thread
    }
}

void mutex_unlock(mutex_t *m) {
    __asm__ __volatile__("cli");
    if (m->locked && m->owner_pid == current_process) {
        if (m->wait_head < m->wait_tail) {
            // Wake up the next waiting thread
            int next_pid = m->waiting_queue[m->wait_head++];
            process_table[next_pid].state = PROC_STATE_READY;
            m->owner_pid = next_pid; // Transfer ownership
            // Don't set locked = 0, we just handed the lock to next_pid
        } else {
            m->locked = 0;
            m->owner_pid = -1;
        }
    }
    __asm__ __volatile__("sti");
}
