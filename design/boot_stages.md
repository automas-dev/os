# Boot Stages

<!-- Table of Contents only links to level 2 headers -->
\[ [Stage 0 - BIOS](#stage-0---bios) \]
\[ [Stage 1 - Boot](#stage-1---boot) \]
\[ [Stage 2 - Loader](#stage-2---loader) \]
\[ [Stage 3 - OS](#stage-3---os) \]

This describes the planned stages and steps, it does not represent the current
implementation or progress.

## Stage 0 - BIOS

The BIOS firmware does a lot more than this, only the steps to launch the kernel
are included.

1. Power On Self Test (POST)
2. Load boot sector (512 Bytes) into memory at 0x7c00
3. Jump to 0x7c00

## Stage 1 - Boot

Execution of the first 512 bytes "boot sector".

1. Store boot drive id from `dl` register
2. Set stack `sp` and base `bp` pointers to 0x6fff
3. Read memory map to 0x500
   1. Detect and store lower memory size using `int 0x12`
   2. Detect upper memory regions using `int 0x15` with aex = 0xe820
4. Read stage 2 from boot drive to 0x7e00
5. Setup GDT (kernel)
6. Switch to protected mode
7. Jump to loader at 0x7e00

## Stage 2 - Loader

Loader starts in raw memory before paging is enabled. After paging is enabled,
initialize the kernel then load and launch init program.

1. Setup kernel logging (serial only)
2. Load VGA driver and clear screen
3. Initialize ram table (physical memory)
4. Initialize kernel virtual memory
   1. Create page dir
   2. Create first page table
   3. Map first page table
   4. Map last table to dir for access to tables
5. Initialize GDT
6. Initialize TSS
7. Enable paging
8. Initialize kernel (**kernel_init**)
   1. Clear kernel struct
   2. Install ISR and IDT
   3. Setup System Calls
   4. Initialize **kmalloc**
   5. (TMP) Setup Event Bus
   6. Create Process Manager
   7. Initialize Scheduler
   8. Install IRQ
      1. Enable interrupts
      2. Timer
      3. Keyboard
      4. ATA
      5. RTC
   9.  Enable time in kernel logs
   10. Mount disk
   11. Mount filesystem
9.  Load init executable
10. Launch init (os main function)

TODO after here is out of date and needs to be rewritten

## Stage 3 - OS

1. Load drivers
   1. VGA / 2d graphics
   2. Keyboard
   3. Disk
   4. Filesystem
   5. RTC
   6. etc.
2. Mount os drive / partition? (does this happen in stage 2?)
3. Setup kernel service calls
4. Begin user space applications loop
   1. Create page directory
   2. Load elf binary
   3. Switch to Ring 3
5. Shell
