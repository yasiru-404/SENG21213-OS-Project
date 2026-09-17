#ifndef PMM_H
#define PMM_H

#include "types.h"

#define PMM_BLOCK_SIZE 4096
#define PMM_BLOCKS_PER_BYTE 8

// E820 Memory Map entry
typedef struct {
    uint32_t base_lo;
    uint32_t base_hi;
    uint32_t length_lo;
    uint32_t length_hi;
    uint32_t type;
    uint32_t acpi_attrs;
} __attribute__((packed)) e820_entry_t;

void pmm_init(void);
void *pmm_alloc_frame(void);
void pmm_free_frame(void *paddr);

uint32_t pmm_get_total_mb(void);
uint32_t pmm_get_used_mb(void);
uint32_t pmm_get_free_mb(void);

#endif
