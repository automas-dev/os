# Boot Stage

Execution of the first 512 bytes "boot sector".

1. Store boot drive id from `dl` register
2. Set stack `sp` and base `bp` pointers to `0x6fff`
3. Read memory map to `0x500`
   1. Detect and store lower memory size using `int 0x12`
   2. Detect upper memory regions using `int 0x15` with `aex = 0xe820`
4. Read stage 2 from boot drive to `0x7e00`
5. Setup GDT (kernel)
6. Switch to protected mode
7. Jump to loader at `0x7e00`
