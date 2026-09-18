#include "../include/ramdisk.h"

// 1 MB array in BSS
static uint8_t ramdisk_data[RAMDISK_SIZE];

void ramdisk_init(void) {
    for (int i = 0; i < RAMDISK_SIZE; i++) {
        ramdisk_data[i] = 0;
    }
}

void ramdisk_read(uint32_t block_no, void *buf) {
    if (block_no >= (RAMDISK_SIZE / RD_BLOCK_SIZE)) return;
    
    uint8_t *src = &ramdisk_data[block_no * RD_BLOCK_SIZE];
    uint8_t *dst = (uint8_t *)buf;
    for (int i = 0; i < RD_BLOCK_SIZE; i++) {
        dst[i] = src[i];
    }
}

void ramdisk_write(uint32_t block_no, const void *buf) {
    if (block_no >= (RAMDISK_SIZE / RD_BLOCK_SIZE)) return;
    
    uint8_t *dst = &ramdisk_data[block_no * RD_BLOCK_SIZE];
    const uint8_t *src = (const uint8_t *)buf;
    for (int i = 0; i < RD_BLOCK_SIZE; i++) {
        dst[i] = src[i];
    }
}
