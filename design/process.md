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

TODO : parent pid

## Ring Scheduler

There needs to be some intermediate task scheduler until a more complete one is
implemented (similar to how malloc needed an intermediate). This intermediate
scheduler will be a ring scheduler.

Each process is in a linked list with a pointer to the next process (and
previous for list removal). When a task switch is performed, the next process
in checked for fulfillment of the event filter. This is repeated until a ready
to run task is found.

> [!WARNING] There must be at least one process ready to run
>
> There must always be a "next" task ready to run. In the case where the current
> process is yielding with an event filter, another process must be ready to
> launch. This will most likely be init, so there needs to be some documentation
> for init behavior written which includes that it must never yield with an
> event filter.

### Process Manager

has pointer to first and foreground processes

### Searching for PID

starts at foreground task

### Event Filter Fulfillment

When the task filter is 0 or there is an event with matching id to filter type.


# Old

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
