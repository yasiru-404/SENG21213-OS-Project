#include "process.h"
#include "idt.h"

extern pcb_t process_table[MAX_PROCESSES];
extern int current_process;
extern int process_count;

static int needs_reschedule = 0;

void scheduler_tick(registers_t *r) {
    (void)r;
    if (process_count > 0) {
        needs_reschedule = 1;
    }
}

void scheduler_yield(void) {
    needs_reschedule = 1;
    __asm__ __volatile__("int $0x20"); // Trigger IRQ0 software interrupt
}

// Called from irq_common_stub in isr.asm
uint32_t scheduler_switch_context(uint32_t esp) {
    if (!needs_reschedule) {
        return esp;
    }
    needs_reschedule = 0;

    // Save current process ESP if one is running
    if (current_process >= 0 && process_table[current_process].state == PROC_STATE_RUNNING) {
        process_table[current_process].esp = esp;
        process_table[current_process].state = PROC_STATE_READY;
    } else if (current_process >= 0 && process_table[current_process].state == PROC_STATE_TERMINATED) {
        // Just exited, nothing to save, let it be replaced eventually
    }

    // Round-robin scheduling
    int next_process = current_process;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        next_process = (next_process + 1) % MAX_PROCESSES;
        if (process_table[next_process].state == PROC_STATE_READY) {
            current_process = next_process;
            process_table[current_process].state = PROC_STATE_RUNNING;
            return process_table[current_process].esp;
        }
    }

    // If no processes are ready, just return original esp (meaning we're idle or halted)
    return esp;
}
