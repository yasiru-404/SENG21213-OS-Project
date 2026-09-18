#include "vga.h"
#include "keyboard.h"
#include "../include/types.h"
#include "../include/idt.h"
#include "../include/pit.h"
#include "../include/process.h"
#include "../include/pmm.h"
#include "../include/fs.h"

/* ---------------------------------------------------------------------------
 * Forward declarations of shell commands
 * --------------------------------------------------------------------------*/
static void cmd_help(void);
static void cmd_clear(void);
static void cmd_about(void);
static void cmd_echo(const char *args);
static void cmd_meminfo(void);
static void cmd_ps(void);
static void cmd_race(void);
static void cmd_race_safe(void);
static void cmd_prodcons(void);

/* ---------------------------------------------------------------------------
 * Utility: minimal string helpers (no libc in a freestanding kernel!)
 * --------------------------------------------------------------------------*/
static int k_strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) { a++; b++; }
    return (uint8_t)*a - (uint8_t)*b;
}

static int k_strncmp(const char *a, const char *b, size_t n) {
    while (n-- && *a && (*a == *b)) { a++; b++; }
    return n == (size_t)-1 ? 0 : (uint8_t)*a - (uint8_t)*b;
}

static size_t k_strlen(const char *s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

/* Skip leading spaces */
static const char *k_ltrim(const char *s) {
    while (*s == ' ') s++;
    return s;
}

/* ---------------------------------------------------------------------------
 * Splash Screen
 * --------------------------------------------------------------------------*/
static void print_splash(void) {
    vga_clear(VGA_BLACK);

    /* Top banner box */
    vga_draw_box(0, 0, 7, 80, VGA_LIGHT_MAGENTA);

    vga_set_cursor(1, 2);
    vga_puts_color("  SENG21213-OS  |  Computer Architecture & Operating Systems",
                   VGA_YELLOW, VGA_BLACK);

    vga_set_cursor(2, 2);
    vga_puts_color("  Stage 0: Kernel Foundations", VGA_LIGHT_CYAN, VGA_BLACK);

    vga_set_cursor(3, 2);
    vga_puts_color("  Faculty of Engineering – Department of Software Engineering",
                   VGA_LIGHT_GREY, VGA_BLACK);

    vga_set_cursor(4, 2);
    vga_puts_color("  Built by students, for students.  Type 'help' to begin.",
                   VGA_LIGHT_GREEN, VGA_BLACK);

    vga_set_cursor(5, 2);
    vga_puts_color("  CPU: i686 (32-bit Protected Mode)  |  Display: VGA 80x25",
                   VGA_DARK_GREY, VGA_BLACK);

    vga_set_cursor(8, 0);
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_puts("  Welcome! This kernel was compiled from source and booted entirely\n");
    vga_puts("  from bare metal. There is no Linux or Windows underneath – only\n");
    vga_puts("  the code you and your team write.\n");
    vga_puts("\n");
    vga_puts("  Assignment milestones to implement:\n");
    vga_puts_color("    [L09] ", VGA_YELLOW, VGA_BLACK);
    vga_puts("Process Management  – PCB, ready queue, round-robin scheduler\n");
    vga_puts_color("    [L10] ", VGA_YELLOW, VGA_BLACK);
    vga_puts("Threads & Sync      – kernel threads, mutex, semaphore\n");
    vga_puts_color("    [L11] ", VGA_YELLOW, VGA_BLACK);
    vga_puts("Memory Management   – physical page allocator, virtual memory\n");
    vga_puts_color("    [L12] ", VGA_YELLOW, VGA_BLACK);
    vga_puts("File System         – RAM disk, FAT-like directory structure\n");
    vga_puts("\n");
}

/* ---------------------------------------------------------------------------
 * Shell command implementations
 * --------------------------------------------------------------------------*/
static void cmd_help(void) {
    vga_puts_color("\n  SENG21213-OS Shell Commands\n", VGA_YELLOW, VGA_BLACK);
    vga_puts("  ─────────────────────────────────────────────\n");
    vga_puts("  help    – Show this help message\n");
    vga_puts("  clear   – Clear the screen\n");
    vga_puts("  about   – About this OS and course\n");
    vga_puts("  echo    – Echo text to screen\n");
    vga_puts("  meminfo – Memory map\n");
    vga_puts_color("\n  Milestones (to implement):\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ps      – [L09] List processes\n");
    vga_puts("  race    – [L10] Demo unsafe race condition\n");
    vga_puts("  race_safe - [L10] Demo safe race condition (mutex)\n");
    vga_puts("  prodcons  - [L10] Demo producer/consumer\n");
    vga_puts("  ls      – [L12] List files\n");
    vga_puts("  touch   – [L12] Create file\n");
    vga_puts("  cat     – [L12] Print file contents\n");
    vga_puts("  write   – [L12] Write to file (write <name> <text>)\n");
    vga_puts("  rm      – [L12] Delete file\n");
    vga_puts("  kill    – [L09] Terminate a process\n");
}

static void cmd_clear(void) {
    vga_clear(VGA_BLACK);
}

static void cmd_about(void) {
    vga_puts_color("\n  About SENG21213-OS\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ─────────────────────────────────────────────\n");
    vga_puts("  Architecture : x86 (i686), 32-bit Protected Mode\n");
    vga_puts("  Bootloader   : Custom MBR (NASM)\n");
    vga_puts("  Kernel       : Freestanding C (GCC, no libc)\n");
    vga_puts("  VM Target    : QEMU (qemu-system-i386)\n");
    vga_puts("  Course       : SENG 21213 – Sem 2\n");
    vga_puts("  Reference    : Stallings, OS: Internals & Design Principles\n\n");
}

static void cmd_echo(const char *args) {
    vga_puts("  ");
    vga_puts(args);
    vga_puts("\n");
}

static void cmd_meminfo(void) {
    uint32_t total = pmm_get_total_mb();
    uint32_t used = pmm_get_used_mb();
    uint32_t free = pmm_get_free_mb();

    vga_puts_color("\n  Physical Memory Map (E820)\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ─────────────────────────────────────────────\n");
    
    char buf[128];
    // Poor man's sprintf
    vga_puts("  Total RAM : ");
    buf[0] = (total / 100) ? '0' + (total / 100) : ' ';
    buf[1] = ((total / 10) % 10) ? '0' + ((total / 10) % 10) : ' ';
    buf[2] = '0' + (total % 10);
    buf[3] = ' '; buf[4] = 'M'; buf[5] = 'B'; buf[6] = '\n'; buf[7] = 0;
    vga_puts(buf);

    vga_puts("  Used  RAM : ");
    buf[0] = (used / 100) ? '0' + (used / 100) : ' ';
    buf[1] = ((used / 10) % 10) ? '0' + ((used / 10) % 10) : ' ';
    buf[2] = '0' + (used % 10);
    vga_puts(buf);

    vga_puts("  Free  RAM : ");
    buf[0] = (free / 100) ? '0' + (free / 100) : ' ';
    buf[1] = ((free / 10) % 10) ? '0' + ((free / 10) % 10) : ' ';
    buf[2] = '0' + (free % 10);
    vga_puts(buf);
    vga_puts("\n");
}

extern void process_print_list(void (*puts_fn)(const char *), void (*puts_col_fn)(const char *, uint8_t, uint8_t));

static void cmd_ps(void) {
    process_print_list(vga_puts, vga_puts_color);
}

extern void demo_race_unsafe(void);
extern void demo_race_safe(void);
extern void demo_prodcons(void);

static void cmd_race(void) {
    demo_race_unsafe();
}

static void cmd_race_safe(void) {
    demo_race_safe();
}

static void cmd_prodcons(void) {
    demo_prodcons();
}

/* ---------------------------------------------------------------------------
 * Shell process
 * --------------------------------------------------------------------------*/
static char  shell_buf[256];
static char  prompt[] = "\n  ksh> ";

static void shell_run(void) {
    vga_puts_color("\n  Kernel Shell ready. Type 'help' for commands.\n",
                   VGA_LIGHT_GREEN, VGA_BLACK);

    while (true) {
        vga_puts_color(prompt, VGA_LIGHT_GREEN, VGA_BLACK);
        kb_readline(shell_buf, sizeof(shell_buf));

        /* Trim leading whitespace */
        const char *cmd = k_ltrim(shell_buf);
        if (k_strlen(cmd) == 0) continue;

        /* Dispatch */
        if (k_strcmp(cmd, "help")  == 0) { cmd_help();  continue; }
        if (k_strcmp(cmd, "clear") == 0) { cmd_clear(); continue; }
        if (k_strcmp(cmd, "about") == 0) { cmd_about(); continue; }
        if (k_strcmp(cmd, "meminfo") == 0) { cmd_meminfo(); continue; }
        if (k_strcmp(cmd, "ps")    == 0) { cmd_ps();    continue; }
        if (k_strcmp(cmd, "race")  == 0) { cmd_race();  continue; }
        if (k_strcmp(cmd, "race_safe") == 0) { cmd_race_safe(); continue; }
        if (k_strcmp(cmd, "prodcons") == 0) { cmd_prodcons(); continue; }

        if (k_strncmp(cmd, "echo ", 5) == 0) {
            cmd_echo(k_ltrim(cmd + 5));
            continue;
        }

        if (k_strcmp(cmd, "ls")    == 0) { cmd_fs_ls(); continue; }
        if (k_strncmp(cmd, "touch ", 6) == 0) { cmd_fs_touch(k_ltrim(cmd + 6)); continue; }
        if (k_strncmp(cmd, "cat ", 4) == 0)   { cmd_fs_cat(k_ltrim(cmd + 4)); continue; }
        if (k_strncmp(cmd, "rm ", 3) == 0)    { cmd_fs_rm(k_ltrim(cmd + 3)); continue; }
        
        if (k_strncmp(cmd, "write ", 6) == 0) {
            const char *args = k_ltrim(cmd + 6);
            char name[28];
            int i = 0;
            while (args[i] && args[i] != ' ' && i < 27) {
                name[i] = args[i];
                i++;
            }
            name[i] = 0;
            const char *text = "";
            if (args[i] == ' ') text = k_ltrim(args + i);
            cmd_fs_write(name, text);
            continue;
        }

        /* Milestone stubs */
        if (k_strcmp(cmd, "kill")    == 0 ||
            k_strcmp(cmd, "threads") == 0 ||
            k_strcmp(cmd, "free")    == 0 ||
            k_strcmp(cmd, "ls")      == 0 ||
            k_strcmp(cmd, "cat")     == 0) {
            vga_puts_color("  [TODO] This command is not yet implemented.\n",
                           VGA_YELLOW, VGA_BLACK);
            vga_puts("  Implement it as part of your lecture assignment.\n");
            continue;
        }

        vga_puts_color("  Unknown command: ", VGA_LIGHT_RED, VGA_BLACK);
        vga_puts(cmd);
        vga_puts("\n  Type 'help' for a list of commands.\n");
    }
}

/* ---------------------------------------------------------------------------
 * Kernel entry point – called from kernel_entry.asm
 * --------------------------------------------------------------------------*/
void kernel_main(void) {
    vga_init();
    
    // Parse E820 map and init Physical Memory Manager
    pmm_init();

    // Initialize File System (RAM Disk)
    fs_init();

    idt_init();
    pit_init(100); // 100 Hz timer
    
    kb_init();
    print_splash();
    
    scheduler_init();
    create_process(shell_run);
    
    // I don't call shell_run directly anymore.
    // Instead I enable interrupts and let the scheduler pick it up.

    while (1) {
        __asm__ __volatile__("hlt");
    }
}
