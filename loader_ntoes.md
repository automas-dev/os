# Loader

Boot loader
- Asm 512 bytes
- Has open disk to read from (/boot)
- Can only read a small amount from disk
- Limited to lower (16-bit) memory range
- Setup gdt (limited)

Loader
- Read from disk by boot loader
- Second stage
- Setup gdt (full) & tss
- Setup interrupts / paging
  - isr / irq
- Mount filesystem and read init
- Has a main function (`__start`)
- Calls kernel init (but kernel should not have a main)
- Load first process (init)
  - Jump to process instead of kernel (has no main)

Kernel
- Currently bundled with loader
- Should have an init but not a main
  - main is the first process (init)
  - kernel acts as a "library" of functions for system interaction

Init

Shell

Applications

# Questions

Where does loader put kernel (in virtual space)?

What happens to memory used by loader?

Should the loader be the full kernel? How big should the kernel be?
