#include "../include/thread.h"
#include "../include/process.h"

extern pcb_t process_table[MAX_PROCESSES];
extern uint8_t process_stacks[MAX_PROCESSES][STACK_SIZE];
extern int process_count;
static uint32_t next_tid = 100; // Just to distinguish from PIDs

// Helper to terminate a thread when its function returns
static void thread_exit(void) {
    process_exit();
}

int thread_create(void (*fn)(void *), void *arg) {
    int pid = -1;
    // Disable interrupts to avoid race conditions during creation
    __asm__ __volatile__("cli");
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROC_STATE_TERMINATED) {
            pid = i;
            break;
        }
    }

    if (pid == -1) {
        __asm__ __volatile__("sti");
        return -1; // No free slots
    }

    pcb_t *pcb = &process_table[pid];
    pcb->pid = next_tid++;
    pcb->state = PROC_STATE_READY;
    pcb->entry_point = fn;
    pcb->stack_base = (uint32_t)&process_stacks[pid][STACK_SIZE];
    
    uint32_t *stack = (uint32_t *)pcb->stack_base;

    // Set up standard C function call stack layout for after IRET
    // [arg]
    // [return address]  (thread_exit)
    *(--stack) = (uint32_t)arg;
    *(--stack) = (uint32_t)thread_exit;
    
    // IRET frame
    *(--stack) = 0x202; // EFLAGS (interrupts enabled)
    *(--stack) = 0x08;  // CS (Kernel Code Segment)
    *(--stack) = (uint32_t)fn; // EIP

    // PUSHA frame
    *(--stack) = 0; // EAX
    *(--stack) = 0; // ECX
    *(--stack) = 0; // EDX
    *(--stack) = 0; // EBX
    *(--stack) = 0; // ESP
    *(--stack) = 0; // EBP
    *(--stack) = 0; // ESI
    *(--stack) = 0; // EDI

    // Segment registers
    *(--stack) = 0x10; // DS
    *(--stack) = 0x10; // ES
    *(--stack) = 0x10; // FS
    *(--stack) = 0x10; // GS

    pcb->esp = (uint32_t)stack;

    process_count++;
    __asm__ __volatile__("sti");
    return pid;
}
