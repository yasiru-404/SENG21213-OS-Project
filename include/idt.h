#ifndef IDT_H
#define IDT_H

#include "types.h"

// IDT Entry Structure
typedef struct {
    uint16_t base_lo;
    uint16_t sel;
    uint8_t  always0;
    uint8_t  flags;
    uint16_t base_hi;
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_ptr_t;

// CPU state pushed by our ISRs
typedef struct {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pushad
    uint32_t int_no, err_code;                       // Interrupt number and error code
    uint32_t eip, cs, eflags, useresp, ss;           // Pushed by CPU automatically
} registers_t;

void idt_init(void);
void irq_install_handler(int irq, void (*handler)(registers_t *r));
void irq_uninstall_handler(int irq);

#endif
