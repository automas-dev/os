# Process

The process tracks and manages the following information.

- Process Id
- Heap Pages
- Stack Pages
- Page Directory + Tables
- Segment Selector
- Stack Pointer
- Registers
- Signal callbacks
- Link to next process
- *TBD Stats about process

## Creating a Process

Each process needs a page directory, heap and stack.

1. Create a page dir
2. Load proc cr3 into temp page
3. Clear dir
4. Map first page to kernel page
5. Setup Stack
   1. Set proc field for stack address
   2. Add page for stack
6. Setup Heap
   1. Set heap start
   2. Add page for heap if needed
7. Free from temp page

## Switch Task

A task or process switch takes advantage of the stacks in different paging
directories to maintain some of the state, so the process doesn't need to. Each
process has it's own page directory and stack there within. When switching
process, all that needs to change is the stack pointer and the page directory.
The TSS entry will need to be updated with the new process' esp0.

1. Save current process
   1. Push any registers to be saved
   2. Save esp to process
2. Load new process
   1. Load esp
   2. Update esp0 of tss
   3. Change cr3 if needed
   4. Pop any registers that were saved

TODO : the ESP0 might be better stored in the kernel instead of the process if
the process page dir does not include a stack for the kernel (eg. isr stack).

# Old

## Process Struct

| Start | Size | Description              |
| ----- | ---- | ------------------------ |
| 0     | 4    | Process Id (PID)         |
| 4     | 4    | Next Heap Page           |
| 8     | 4    | Stack Page Count         |
| 16    | 4    | Page Directory (CR3)     |
| 12    | 4    | Stack Pointer (ESP)      |
| 20    | 4    | ISR Stack Pointer (ESP0) |
| 24    | 4    | Signal Callback Function |
| 28    | 4    | Pointer to next Process  |

- `Size` is in bytes.
- PID 0 is reserved for the kernel, the first proc will be pid 1

## Memory

Each process is designated a unique page directory. The first table is always
mapped to the kernel (4 MB), but the remaining space is available for user space
programs.

| start      | end        | pages       | description             |
| ---------- | ---------- | ----------- | ----------------------- |
| 0x00000000 | 0x003fffff | 0x400       | _Kernel Pages_          |
| 0x00400000 | x          | n           | Program and Heap        |
| x          | 0xffffefff | 0xffbff - n | User Stack (grows down) |
| 0xfffff000 | 0xffffffff | 0x00010     | ISR Stack (grows down)  |
