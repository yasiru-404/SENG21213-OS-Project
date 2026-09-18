#include "../include/fs.h"
#include "../include/ramdisk.h"
#include "../include/vga.h"

#define MAX_INODES 64
#define MAX_FILES 64
#define FS_TOTAL_BLOCKS 256

// Block layout:
// 0: Superblock
// 1: Directory entries (64 entries * 32 bytes = 2048 bytes)
// 2: Inode bitmap (1 block)
// 3: Block bitmap (1 block)
// 4: Inodes (64 * 36 bytes = 2304 bytes)
// 5-255: Data blocks

#define BLOCK_SUPER 0
#define BLOCK_DIR   1
#define BLOCK_IBIT  2
#define BLOCK_BBIT  3
#define BLOCK_INODE 4
#define BLOCK_DATA_START 5

static uint8_t fs_buf[RD_BLOCK_SIZE]; // Temporary block buffer

// Very simple Open File Table
typedef struct {
    uint32_t inode_num;
    uint32_t offset;
    int in_use;
} oft_entry_t;

#define MAX_OPEN_FILES 16
static oft_entry_t open_files[MAX_OPEN_FILES];

// Helper: strings
static int k_strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) { a++; b++; }
    return (uint8_t)*a - (uint8_t)*b;
}

static void k_strcpy(char *dest, const char *src) {
    while ((*dest++ = *src++));
}

static size_t k_strlen(const char *s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

void fs_format(void) {
    ramdisk_init();
    
    // Superblock
    superblock_t *sb = (superblock_t *)fs_buf;
    for (int i=0; i<RD_BLOCK_SIZE; i++) fs_buf[i] = 0;
    sb->magic = FS_MAGIC;
    sb->total_blocks = FS_TOTAL_BLOCKS;
    sb->total_inodes = MAX_INODES;
    sb->free_blocks = FS_TOTAL_BLOCKS - BLOCK_DATA_START;
    sb->free_inodes = MAX_INODES - 1; // inode 0 is reserved
    ramdisk_write(BLOCK_SUPER, fs_buf);

    // Directory
    for (int i=0; i<RD_BLOCK_SIZE; i++) fs_buf[i] = 0;
    ramdisk_write(BLOCK_DIR, fs_buf);

    // Inode bitmap (mark inode 0 used)
    for (int i=0; i<RD_BLOCK_SIZE; i++) fs_buf[i] = 0;
    fs_buf[0] = 0x01;
    ramdisk_write(BLOCK_IBIT, fs_buf);

    // Block bitmap (mark blocks 0 to 4 used)
    for (int i=0; i<RD_BLOCK_SIZE; i++) fs_buf[i] = 0;
    fs_buf[0] = 0x1F; // binary 0001 1111
    ramdisk_write(BLOCK_BBIT, fs_buf);

    // Inodes
    for (int i=0; i<RD_BLOCK_SIZE; i++) fs_buf[i] = 0;
    ramdisk_write(BLOCK_INODE, fs_buf);
}

void fs_init(void) {
    ramdisk_init();
    ramdisk_read(BLOCK_SUPER, fs_buf);
    superblock_t *sb = (superblock_t *)fs_buf;
    if (sb->magic != FS_MAGIC) {
        fs_format();
    }
    for (int i=0; i<MAX_OPEN_FILES; i++) {
        open_files[i].in_use = 0;
    }
}

// Allocates a free inode, returns inode number (1 to MAX_INODES-1)
static uint32_t alloc_inode(void) {
    uint8_t bitmap[RD_BLOCK_SIZE];
    ramdisk_read(BLOCK_IBIT, bitmap);
    for (int i = 1; i < MAX_INODES; i++) {
        if ((bitmap[i / 8] & (1 << (i % 8))) == 0) {
            bitmap[i / 8] |= (1 << (i % 8));
            ramdisk_write(BLOCK_IBIT, bitmap);
            return i;
        }
    }
    return 0;
}

// Allocates a free data block, returns block number
static uint32_t alloc_block(void) {
    uint8_t bitmap[RD_BLOCK_SIZE];
    ramdisk_read(BLOCK_BBIT, bitmap);
    for (int i = BLOCK_DATA_START; i < FS_TOTAL_BLOCKS; i++) {
        if ((bitmap[i / 8] & (1 << (i % 8))) == 0) {
            bitmap[i / 8] |= (1 << (i % 8));
            ramdisk_write(BLOCK_BBIT, bitmap);
            return i;
        }
    }
    return 0;
}

static void free_block(uint32_t b) {
    uint8_t bitmap[RD_BLOCK_SIZE];
    ramdisk_read(BLOCK_BBIT, bitmap);
    bitmap[b / 8] &= ~(1 << (b % 8));
    ramdisk_write(BLOCK_BBIT, bitmap);
}

static void free_inode(uint32_t inum) {
    uint8_t bitmap[RD_BLOCK_SIZE];
    ramdisk_read(BLOCK_IBIT, bitmap);
    bitmap[inum / 8] &= ~(1 << (inum % 8));
    ramdisk_write(BLOCK_IBIT, bitmap);
}

int fs_open(const char *name, int create) {
    dir_entry_t dir[MAX_FILES];
    ramdisk_read(BLOCK_DIR, dir);

    uint32_t target_inode = 0;
    int free_dir_idx = -1;

    for (int i = 0; i < MAX_FILES; i++) {
        if (dir[i].inode_num == 0) {
            if (free_dir_idx == -1) free_dir_idx = i;
        } else if (k_strcmp(dir[i].name, name) == 0) {
            target_inode = dir[i].inode_num;
            break;
        }
    }

    if (target_inode == 0) {
        if (!create || free_dir_idx == -1) return -1;
        target_inode = alloc_inode();
        if (target_inode == 0) return -1;
        k_strcpy(dir[free_dir_idx].name, name);
        dir[free_dir_idx].inode_num = target_inode;
        ramdisk_write(BLOCK_DIR, dir);
        
        // Init inode
        inode_t inodes[MAX_INODES];
        ramdisk_read(BLOCK_INODE, inodes);
        inodes[target_inode].size = 0;
        for (int i=0; i<8; i++) inodes[target_inode].blocks[i] = 0;
        ramdisk_write(BLOCK_INODE, inodes);
    }

    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (!open_files[i].in_use) {
            open_files[i].inode_num = target_inode;
            open_files[i].offset = 0;
            open_files[i].in_use = 1;
            return i;
        }
    }
    return -1;
}

void fs_close(int fd) {
    if (fd >= 0 && fd < MAX_OPEN_FILES) {
        open_files[fd].in_use = 0;
    }
}

int fs_write(int fd, const void *buf, int count) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || !open_files[fd].in_use) return -1;
    if (count <= 0) return 0;

    inode_t inodes[MAX_INODES];
    ramdisk_read(BLOCK_INODE, inodes);
    uint32_t inum = open_files[fd].inode_num;
    inode_t *node = &inodes[inum];

    // Simple append only for now, up to 1 block
    if (open_files[fd].offset + count > RD_BLOCK_SIZE) {
        count = RD_BLOCK_SIZE - open_files[fd].offset; 
    }
    
    if (count == 0) return 0;

    if (node->blocks[0] == 0) {
        node->blocks[0] = alloc_block();
        if (node->blocks[0] == 0) return -1;
    }

    uint8_t databuf[RD_BLOCK_SIZE];
    ramdisk_read(node->blocks[0], databuf);
    
    const uint8_t *src = (const uint8_t *)buf;
    for (int i = 0; i < count; i++) {
        databuf[open_files[fd].offset + i] = src[i];
    }
    ramdisk_write(node->blocks[0], databuf);

    open_files[fd].offset += count;
    if (open_files[fd].offset > node->size) {
        node->size = open_files[fd].offset;
    }
    
    ramdisk_write(BLOCK_INODE, inodes);
    return count;
}

int fs_read(int fd, void *buf, int count) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || !open_files[fd].in_use) return -1;
    
    inode_t inodes[MAX_INODES];
    ramdisk_read(BLOCK_INODE, inodes);
    uint32_t inum = open_files[fd].inode_num;
    inode_t *node = &inodes[inum];

    if (open_files[fd].offset >= node->size) return 0;

    if (open_files[fd].offset + count > node->size) {
        count = node->size - open_files[fd].offset;
    }

    if (node->blocks[0] == 0) return 0;

    uint8_t databuf[RD_BLOCK_SIZE];
    ramdisk_read(node->blocks[0], databuf);
    
    uint8_t *dst = (uint8_t *)buf;
    for (int i = 0; i < count; i++) {
        dst[i] = databuf[open_files[fd].offset + i];
    }

    open_files[fd].offset += count;
    return count;
}

int fs_unlink(const char *name) {
    dir_entry_t dir[MAX_FILES];
    ramdisk_read(BLOCK_DIR, dir);

    for (int i = 0; i < MAX_FILES; i++) {
        if (dir[i].inode_num != 0 && k_strcmp(dir[i].name, name) == 0) {
            uint32_t inum = dir[i].inode_num;
            dir[i].inode_num = 0; // Remove from dir
            ramdisk_write(BLOCK_DIR, dir);

            inode_t inodes[MAX_INODES];
            ramdisk_read(BLOCK_INODE, inodes);
            for (int j=0; j<8; j++) {
                if (inodes[inum].blocks[j] != 0) {
                    free_block(inodes[inum].blocks[j]);
                }
            }
            free_inode(inum);
            return 0;
        }
    }
    return -1;
}

// ---------------------------------------------------------------------------
// Shell Commands
// ---------------------------------------------------------------------------
void cmd_fs_ls(void) {
    dir_entry_t dir[MAX_FILES];
    inode_t inodes[MAX_INODES];
    ramdisk_read(BLOCK_DIR, dir);
    ramdisk_read(BLOCK_INODE, inodes);

    vga_puts_color("\n  File           Size\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ─────────────────────────────────────────────\n");
    int count = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (dir[i].inode_num != 0) {
            vga_puts("  ");
            vga_puts(dir[i].name);
            
            // Pad spaces
            int len = k_strlen(dir[i].name);
            for(int s=0; s<15-len; s++) vga_puts(" ");
            
            // Print size
            uint32_t sz = inodes[dir[i].inode_num].size;
            char buf[8];
            buf[0] = (sz / 1000) ? '0' + (sz / 1000) : ' ';
            buf[1] = ((sz / 100) % 10) ? '0' + ((sz / 100) % 10) : ((sz>99)?'0':' ');
            buf[2] = ((sz / 10) % 10) ? '0' + ((sz / 10) % 10) : ((sz>9)?'0':' ');
            buf[3] = '0' + (sz % 10);
            buf[4] = ' '; buf[5] = 'B'; buf[6] = '\n'; buf[7] = 0;
            vga_puts(buf);
            count++;
        }
    }
    if (count == 0) vga_puts("  <empty>\n");
    vga_puts("\n");
}

void cmd_fs_touch(const char *name) {
    int fd = fs_open(name, 1);
    if (fd >= 0) fs_close(fd);
    else vga_puts("  Error creating file.\n");
}

void cmd_fs_cat(const char *name) {
    int fd = fs_open(name, 0);
    if (fd < 0) {
        vga_puts("  File not found.\n");
        return;
    }
    char buf[128];
    int bytes;
    vga_puts("  ");
    while ((bytes = fs_read(fd, buf, sizeof(buf)-1)) > 0) {
        buf[bytes] = '\0';
        vga_puts(buf);
    }
    vga_puts("\n");
    fs_close(fd);
}

void cmd_fs_write(const char *name, const char *text) {
    int fd = fs_open(name, 1);
    if (fd < 0) {
        vga_puts("  Error opening file.\n");
        return;
    }
    fs_write(fd, text, k_strlen(text));
    fs_close(fd);
}

void cmd_fs_rm(const char *name) {
    if (fs_unlink(name) == 0) {
        vga_puts("  File deleted.\n");
    } else {
        vga_puts("  File not found.\n");
    }
}
