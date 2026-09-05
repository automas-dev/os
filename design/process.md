# Process

<!-- Table of Contents only links to level 2 headers -->
\[ [Creating a Process](#creating-a-process) \]
\[ [Switch Task](#switch-task) \]
\[ [Ring 3 / User Space Execution](#ring-3--user-space-execution) \]
\[ [Ring Scheduler](#ring-scheduler) \]
<!-- Ignoring because it's under # Old -->
<!-- \[ [Memory](#memory) \] -->

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
- (TBD) Stats about process

## Creating a Process

Each process needs a page directory, an ISR (kernel-mode) stack, a user (ring 3)
stack and a heap.

1. Create a page dir
2. Load proc cr3 into temp page
3. Clear dir
4. Map first page to kernel page
5. Setup ISR stack
   1. Set proc field for ISR stack address (`esp` / `esp0`, see [Switch Task](#switch-task))
   2. Add pages for ISR stack, **supervisor-only** (`MMU_TABLE_RW`)
6. Setup user stack
   1. Add first page for the ring 3 user stack, **user-accessible** (`MMU_TABLE_RW_USER`)
7. Setup Heap
   1. Set heap start
   2. Add page for heap if needed, **user-accessible** (`MMU_TABLE_RW_USER`)
8. Free from temp page

> [!IMPORTANT]
> Every page a process can allocate after creation (heap growth via
> `process_add_pages`, additional user stack pages via `process_grow_stack`)
> must be mapped `MMU_TABLE_RW_USER`, not `MMU_TABLE_RW`, or ring 3 code will
> page fault trying to use it. The only pages that stay supervisor-only are the
> shared kernel table (table 0) and each process' own ISR stack.
>
> A process' own heap (used for `malloc`/`realloc`/`free`) is entirely managed
> in user space — see [src/libc/src/memory_alloc.c](../src/libc/src/memory_alloc.c)
> and [src/libc/src/memory.c](../src/libc/src/memory.c) — since libc is
> statically linked into every process' own binary, each process automatically
> gets its own private heap allocator state simply by living in its own
> address space. The kernel is only ever involved to grow that heap: a single
> system call (`SYS_CALL_MEM_ALLOC_PAGE`) maps `count` more pages onto the end
> of the process' heap (via `process_add_pages`) and returns a pointer to the
> first of them.

Any kernel-side data that must be handed back to a process (eg. a string
returned by a system call) has to be copied into that process' own heap first
(`process_copy_to_heap`) — pointers into kernel memory (string literals,
`kmalloc`'d buffers, etc.) are not legal for ring 3 code to dereference.

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

`proc->esp` always refers to the process' **kernel-mode / ISR stack** pointer
(the one `switch_task` saves and restores), not its ring 3 user stack pointer.
The user stack pointer only matters for the very first launch of a process
(see [Ring 3 / User Space Execution](#ring-3--user-space-execution)); after
that, it is saved and restored automatically as part of the normal interrupt
return (`iret`) from every subsequent system call or interrupt, the same way
`esp`/`eip`/`cs`/etc. always are.

TODO : the ESP0 might be better stored in the kernel instead of the process if
the process page dir does not include a stack for the kernel (eg. isr stack).

TODO : parent pid

## Ring 3 / User Space Execution

Every process actually executes at CPL 3 (ring 3 / user space). The kernel
(ring 0) is only entered through interrupts: hardware IRQs, CPU exceptions, and
the system call interrupt (`int 0x30`, see [system calls](system_call.md)).
This relies on infrastructure set up once at boot (`init_gdt`, `init_tss`,
`isr_install`) plus a construct performed once per process (its first launch).

### One-Time Setup

- The GDT already defines ring 3 code/data segments
  (`GDT_SELECTOR_USER_CODE`/`GDT_SELECTOR_USER_DATA` in `cpu/gdt.h`).
- The TSS's `ss0` is set to the kernel data selector, and `esp0` is kept in
  sync with the active process' own ISR stack on every `switch_task` (see
  [Switch Task](#switch-task)). Together these tell the CPU exactly which
  stack (segment + pointer) to switch to on any ring 3 → ring 0 transition.
- Every IDT gate is DPL 0 (kernel-only) **except** the system call interrupt
  (vector 48 / `int 0x30`), which is DPL 3 so ring 3 code can trap into the
  kernel directly. Hardware IRQs and CPU exceptions can still reach a DPL 0
  gate regardless of the current ring, so this is the only gate that needs
  widening.

### First Launch (`process_set_entrypoint`)

There is no real "previous trap" to return to the first time a process runs,
so a fake one is constructed:

> [!IMPORTANT]
> The loader/exec code always passes `VADDR_USER_MEM` (the very first byte of
> the loaded flat binary) as the entrypoint — there is no ELF header parsing,
> so whatever code lands at that address is what actually runs first. Every
> app links `cinit`'s `entry.asm` (`__start`) ahead of its own code by placing
> `__start` in a dedicated `.text.entry` section that each app's `link.ld`
> pulls in before the generic `.text` section (`*(.text.entry)` then
> `*(.text)`). `__start` reads `argc`/`argv` off the initial user stack (see
> below) and forwards them via a normal cdecl `call` into `__cinit`, which
> then calls the app's `main`. An app must not define its own competing entry
> symbol or skip linking against `cinit` — doing so changes what the process'
> code actually starts executing at, silently bypassing the `argc`/`argv`
> setup.

1. A real IRET stack frame (`EIP`, `CS`, `EFLAGS`, `ESP`, `SS`) is written to
   the top of the process' ISR stack, targeting the process' entrypoint at
   `CS = GDT_SELECTOR_USER_CODE` / `SS = GDT_SELECTOR_USER_DATA`, with `ESP`
   pointing at the top of the process' ring 3 user stack (below the process'
   `argc`/`argv`, written just below it — see
   [system calls](system_call.md) / `process_copy_args`).
2. Immediately below that frame, a fake "return address" is written pointing
   at a small trampoline, `enter_usermode` (`kernel_entry.asm`).
3. `proc->esp` (the process' ISR stack pointer) is set to point at the start
   of this constructed frame.

The very first time this process is resumed, `switch_task.resume`'s existing,
unmodified `pop eax/esi/edi/ebp; ret` sequence "returns" into `enter_usermode`
exactly as it would return into any real, previously-suspended kernel call
chain. `enter_usermode` loads the ring 3 data selector into the segment
registers (`ss` is restored by `iret` itself) and executes `iret`, which pops
the constructed frame and drops the CPU to ring 3 at the process' entrypoint.

Every *subsequent* suspend/resume of the process needs no special handling:
a system call or interrupt from ring 3 lands on the process' ISR stack (via
the already-current `esp0`), and the existing ISR/IRQ assembly stubs'
own `iret` is what returns to ring 3 — the same mechanism used by every other
interrupt return, kernel code included.

## Ring Scheduler

There needs to be some intermediate task scheduler until a more complete one is
implemented (similar to how malloc needed an intermediate). This intermediate
scheduler will be a ring scheduler.

Each process is in a linked list with a pointer to the next process (and
previous for list removal). When a task switch is performed, the next process
in checked for fulfillment of the event filter. This is repeated until a ready
to run task is found.

> [!WARNING]
> There must always be at least one process (a "next" task) ready to run. In the
> case where the current process is yielding with an event filter, another
> process must be ready to launch or resume.
> 
> This will most likely be init, so documentation is needed for init behavior
> written which includes that it must never yield with an event filter.

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
