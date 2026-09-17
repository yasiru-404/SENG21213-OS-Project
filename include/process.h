#ifndef PROCESS_H
#define PROCESS_H

#include "types.h"

typedef enum {
    PROC_STATE_READY,
    PROC_STATE_RUNNING,
    PROC_STATE_BLOCKED,
    PROC_STATE_TERMINATED
} process_state_t;

typedef struct {
    uint32_t pid;
    process_state_t state;
    uint32_t esp;       // Current stack pointer
    uint32_t stack_base; // Base of allocated 4KB stack
    void *entry_point;
} pcb_t;

#define MAX_PROCESSES 64
#define STACK_SIZE 4096

// Scheduler APIs
void scheduler_init(void);
int create_process(void (*entry_fn)(void));
void scheduler_yield(void);
void process_exit(void);

#endif
