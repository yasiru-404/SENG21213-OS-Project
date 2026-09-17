#include "process.h"

// For stage 1, we use static arrays since PMM isn't implemented until Stage 3
pcb_t process_table[MAX_PROCESSES];
uint8_t process_stacks[MAX_PROCESSES][STACK_SIZE];

static uint32_t next_pid = 1;
int current_process = -1;
int process_count = 0;

void scheduler_init(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_table[i].state = PROC_STATE_TERMINATED;
    }
}

int create_process(void (*entry_fn)(void)) {
    int pid = -1;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROC_STATE_TERMINATED) {
            pid = i;
            break;
        }
    }

    if (pid == -1) return -1; // No free slots

    pcb_t *pcb = &process_table[pid];
    pcb->pid = next_pid++;
    pcb->state = PROC_STATE_READY;
    pcb->entry_point = entry_fn;
    pcb->stack_base = (uint32_t)&process_stacks[pid][STACK_SIZE];
    
    // Set up initial stack frame for the process to simulate an interrupt
    uint32_t *stack = (uint32_t *)pcb->stack_base;

    // iret expects: EIP, CS, EFLAGS
    *(--stack) = 0x202; // EFLAGS (interrupts enabled)
    *(--stack) = 0x08;  // CS (Kernel Code Segment)
    *(--stack) = (uint32_t)entry_fn; // EIP

    // pusha expects: EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX
    *(--stack) = 0; // EAX
    *(--stack) = 0; // ECX
    *(--stack) = 0; // EDX
    *(--stack) = 0; // EBX
    *(--stack) = 0; // ESP (ignored by popa)
    *(--stack) = 0; // EBP
    *(--stack) = 0; // ESI
    *(--stack) = 0; // EDI

    // push ds, es, fs, gs
    *(--stack) = 0x10; // DS
    *(--stack) = 0x10; // ES
    *(--stack) = 0x10; // FS
    *(--stack) = 0x10; // GS

    pcb->esp = (uint32_t)stack;

    process_count++;
    return pid;
}

void process_exit(void) {
    if (current_process >= 0) {
        process_table[current_process].state = PROC_STATE_TERMINATED;
        process_count--;
        while(1) {
            __asm__ __volatile__("hlt"); // Wait for scheduler to switch us out
        }
    }
}

// Add print list for shell ps command
void process_print_list(void (*puts_fn)(const char *), void (*puts_col_fn)(const char *, uint8_t, uint8_t)) {
    puts_col_fn("\n  PID | STATE      | ENTRY\n", 0x0E, 0x00);
    puts_fn("  ─────────────────────────────────────────────\n");
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state != PROC_STATE_TERMINATED) {
            char buf[32];
            // Poor man's itoa
            int pid = process_table[i].pid;
            buf[0] = ' '; buf[1] = ' '; buf[2] = ' '; buf[3] = (pid % 10) + '0';
            if (pid >= 10) buf[2] = (pid / 10) % 10 + '0';
            buf[4] = ' '; buf[5] = '|'; buf[6] = ' '; buf[7] = '\0';
            puts_fn(buf);
            
            if (process_table[i].state == PROC_STATE_RUNNING) puts_fn("RUNNING    | ");
            else if (process_table[i].state == PROC_STATE_READY) puts_fn("READY      | ");
            else puts_fn("BLOCKED    | ");
            
            // Print entry point as hex (simplified)
            uint32_t ep = (uint32_t)process_table[i].entry_point;
            char hex[16] = "0x00000000\n";
            for (int j = 9; j >= 2; j--) {
                int nibble = ep & 0xF;
                hex[j] = nibble < 10 ? '0' + nibble : 'A' + (nibble - 10);
                ep >>= 4;
            }
            puts_fn(hex);
        }
    }
    puts_fn("\n");
}
