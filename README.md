# SENG21213-OS — Project Submission

**Course**: SENG 21213 – Computer Architecture & Operating Systems  
**Student Name**: M. A. Yasiru Sandeepa  
**Student Number**: SE/2023/014  

---

## Project Overview

This is a custom x86 Operating System developed for the SENG 21213 course. The project involves building an OS from scratch, starting from a minimal bootloader and transitioning into a 32-bit protected-mode kernel.

### Implemented Features

Currently, the following features have been successfully implemented:
- **Bootloader**: 16-bit real mode to 32-bit protected mode transition, and Global Descriptor Table (GDT) setup.
- **VGA Driver**: 80×25 text-mode display driver.
- **Keyboard Driver**: PS/2 keyboard polling driver for user input.
- **Interactive Shell**: A basic command-line interface running in the kernel.

---

## Project Structure

```text
seng21213-os/
├── boot/
│   └── boot.asm          ← MBR Bootloader (NASM, 16-bit → 32-bit transition)
├── kernel/
│   ├── kernel_entry.asm  ← Protected-mode entry
│   ├── kernel.c          ← Main kernel, shell loop, and command dispatch
│   ├── vga.c / vga.h     ← VGA text-mode driver
│   ├── keyboard.c / .h   ← PS/2 keyboard driver
├── include/
│   └── types.h           ← Primitive types
├── linker.ld             ← Linker script
├── Makefile              ← Build system
└── Dockerfile            ← Reproducible build environment
```

---

## Quick Start (How to Build and Run)

### Option A: Docker (Recommended)

Using Docker ensures a reproducible build environment across all platforms.

```bash
# 1. Build the Docker image once:
docker build -t seng21213-os-builder .

# 2. Compile the OS using the container:
docker run --rm -v "$(pwd)":/os seng21213-os-builder

# 3. Run the compiled OS image in QEMU:
qemu-system-i386 -drive format=raw,file=seng21213-os.img -m 32M
```

### Option B: Native Linux / WSL2

Ensure you have the required dependencies installed:
```bash
# Ubuntu/Debian dependencies
sudo apt install nasm gcc gcc-multilib binutils qemu-system-x86 make
```

Build and run using the Makefile:
```bash
make all
make run
```

### Option C: macOS (Homebrew)

```bash
brew install nasm x86_64-elf-binutils qemu

# Note: You also need an i686-elf-gcc cross-compiler.
make all
make run
```

---

## Debugging Tips

To run the OS in debug mode with GDB:

```bash
# Terminal 1: Run QEMU in debug mode
make run-debug

# Terminal 2: Connect GDB
gdb
(gdb) target remote :1234
(gdb) set architecture i386
(gdb) symbol-file build/kernel.elf
(gdb) break kernel_main
(gdb) continue
```

To inspect the raw disk image:
```bash
xxd seng21213-os.img | head -32        # View MBR
xxd seng21213-os.img | grep -c aa55    # Verify boot signature
```
