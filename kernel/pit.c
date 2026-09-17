#include "pit.h"
#include "io.h"

void pit_init(uint32_t frequency) {
    uint32_t divisor = 1193180 / frequency;
    outb(0x43, 0x36);             // Command byte
    outb(0x40, (uint8_t)(divisor & 0xFF));   // Low byte
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF)); // High byte
}
