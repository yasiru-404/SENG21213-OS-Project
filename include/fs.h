#ifndef FS_H
#define FS_H

#include "types.h"

#define FS_MAGIC 0x53454E47

typedef struct {
    uint32_t magic;
    uint32_t total_blocks;
    uint32_t total_inodes;
    uint32_t free_blocks;
    uint32_t free_inodes;
} superblock_t;

typedef struct {
    uint32_t size;
    uint32_t blocks[8];
} inode_t;

typedef struct {
    char name[28];
    uint32_t inode_num; // 0 means unused (valid inodes start at 1)
} dir_entry_t;

void fs_init(void);
void fs_format(void);
int fs_open(const char *name, int create); // returns fd, create=1 to create if missing
int fs_read(int fd, void *buf, int count);
int fs_write(int fd, const void *buf, int count);
void fs_close(int fd);
int fs_unlink(const char *name);

// For shell commands
void cmd_fs_ls(void);
void cmd_fs_touch(const char *name);
void cmd_fs_cat(const char *name);
void cmd_fs_write(const char *name, const char *text);
void cmd_fs_rm(const char *name);

#endif
