#include "../include/pmm.h"

// Max memory we'll manage: 128 MB for this simple OS
#define MAX_MEM_MB 128
#define MAX_BLOCKS ((MAX_MEM_MB * 1024 * 1024) / PMM_BLOCK_SIZE)
#define BITMAP_SIZE (MAX_BLOCKS / PMM_BLOCKS_PER_BYTE)

static uint8_t memory_bitmap[BITMAP_SIZE];
static uint32_t total_memory = 0; // in bytes
static uint32_t used_memory = 0;  // in bytes
static uint32_t max_blocks = 0;

static inline void bitmap_set(uint32_t bit) {
    memory_bitmap[bit / 8] |= (1 << (bit % 8));
}

static inline void bitmap_clear(uint32_t bit) {
    memory_bitmap[bit / 8] &= ~(1 << (bit % 8));
}

static inline int bitmap_test(uint32_t bit) {
    return memory_bitmap[bit / 8] & (1 << (bit % 8));
}

void pmm_init(void) {
    uint32_t e820_count = *(uint32_t *)0x8000;
    e820_entry_t *mmap = (e820_entry_t *)0x8004;

    // By default, set all bits to used (1)
    for (uint32_t i = 0; i < BITMAP_SIZE; i++) {
        memory_bitmap[i] = 0xFF;
    }

    total_memory = 0;

    for (uint32_t i = 0; i < e820_count; i++) {
        if (mmap[i].type == 1) { // 1 = Usable RAM
            uint32_t base = mmap[i].base_lo;
            uint32_t length = mmap[i].length_lo;
            
            // Limit to our MAX_MEM_MB
            if (base >= (MAX_MEM_MB * 1024 * 1024)) continue;
            if (base + length > (MAX_MEM_MB * 1024 * 1024)) {
                length = (MAX_MEM_MB * 1024 * 1024) - base;
            }

            total_memory += length;

            uint32_t start_block = base / PMM_BLOCK_SIZE;
            uint32_t num_blocks = length / PMM_BLOCK_SIZE;

            for (uint32_t j = 0; j < num_blocks; j++) {
                bitmap_clear(start_block + j);
            }
            
            if (start_block + num_blocks > max_blocks) {
                max_blocks = start_block + num_blocks;
            }
        }
    }

    // Explicitly reserve first 1MB (BIOS, Kernel, VGA, etc)
    for (uint32_t i = 0; i < (1024 * 1024) / PMM_BLOCK_SIZE; i++) {
        bitmap_set(i);
    }
    
    used_memory = (1024 * 1024); // First 1MB is used
}

void *pmm_alloc_frame(void) {
    for (uint32_t i = 0; i < max_blocks; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            used_memory += PMM_BLOCK_SIZE;
            return (void *)(i * PMM_BLOCK_SIZE);
        }
    }
    return 0; // Out of memory
}

void pmm_free_frame(void *paddr) {
    uint32_t block = (uint32_t)paddr / PMM_BLOCK_SIZE;
    if (bitmap_test(block)) {
        bitmap_clear(block);
        used_memory -= PMM_BLOCK_SIZE;
    }
}

uint32_t pmm_get_total_mb(void) {
    return total_memory / (1024 * 1024);
}

uint32_t pmm_get_used_mb(void) {
    return used_memory / (1024 * 1024);
}

uint32_t pmm_get_free_mb(void) {
    return (total_memory - used_memory) / (1024 * 1024);
}
