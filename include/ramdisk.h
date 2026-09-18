#ifndef RAMDISK_H
#define RAMDISK_H

#include "types.h"

#define RAMDISK_SIZE (1024 * 1024)
#define RD_BLOCK_SIZE 4096

void ramdisk_init(void);
void ramdisk_read(uint32_t block_no, void *buf);
void ramdisk_write(uint32_t block_no, const void *buf);

#endif
