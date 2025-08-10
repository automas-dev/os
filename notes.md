# Notes and ToDos for active and planned future work

This is a list of all active and planned work. They can be removed once they are
checked off. Use the task template bellow for larger tasks that need some design
work or require additional information. Those can be moved under the Completed
section once they are finished.

- [ ] Documentation
  - [x] Add design doc for boot stages and move out of readme
  - [x] Update boot stages to be more accurate
  - [ ] Filesystem documentation
  - [x] Move doc files under design folder
  - [x] Clean up goals in readme
    - [x] Remove completed section (make checked boxes in list)
    - [x] Cleanup Goals term lists
  - [ ] System call arch
- [ ] mmu
  - [ ] free interior pages when malloc free's entire virtual page
- [ ] tar
  - [ ] list directories
- [ ] disk
  - [x] ata driver
  - [ ] floppy driver
  - [ ] ram disk driver
- [ ] test (os built in)
  - [ ] circbuff_rpop
  - [ ] circbuff_read edge case read after remove where start has changed
- [ ] ram
  - [ ] handle region overlap
- [ ] rtc
  - [ ] pretty much everything
- [ ] dir
  - [ ] pretty much everything
- [ ] file
  - [ ] pretty much everything
- [ ] Handle page faults
  - [ ] eg. add pages for user space stack
- [ ] Ring 3
  - [x] load code
  - [ ] add gdt
  - [ ] setup tss
  - [x] kernel interrupts
- [ ] Document code
- [ ] Test code
- [ ] Move drivers and other os level code out of kernel (only keep essentials)

## Getting out of ebus

I think ebus is slowing things down + it doesn't allow for any priority (as
written). Some calls could be way faster passing control directly to the kernel
or next / target process. Ebus could still be useful for cases where the event
cannot be served and must buffer (io, key events, etc.) but exec should use
system calls.

### Tasks

- [ ] Define and document boundary between and usage of
  - [ ] ebus
  - [ ] system call
  - [ ] signals

## IO

Now that shell is working, it needs a way to load and execute programs. Start
with a function that can list a directory, store a cwd in the shell and add
a system call to launch a new process from it's filename and args.

### Tasks

- [ ] System call to list dir
- [ ] PWD for shell
- [ ] System call to launch program from filename and args

## Process End

Add code to handle a process closing or crashing. It should stop and remove the
process from the process manager, free the memory and any ram pages allocated.
The exit modes are "natural" (return from main), "exit" (call to exit()),
"crash" (kernel kill or abort).

### Tasks

- [ ] Handle program exit modes
  - [ ] Natural
  - [ ] Exit
  - [ ] Crash
  - [ ] Kernel Kill
- [ ] Remove from process manager
- [ ] Free process
- [ ] Any signals that may be relevant

## Task Scheduler

Create a task scheduler and add a yield to most / all system calls.

### Tasks

- [ ] Choose what task to run next
  - [ ] Resume process with all events in queue before switching task
  - [ ] Task priority
- [ ] Yield for all system calls

## _Template Task_

Description of task and any relevant details / information.

### Tasks

- [ ] Sub task list

# Completed

## TSS / Process

In an effort to reach ring 3, there are a few required systems to setup.

TSS needs a kernel stack. The space between the vga page and kernel code, there
will exist the kernel stack for TSS / interrupts.

### Tasks

- [x] Setup kernel stack pages bellow VGA memory
  - [x] Assign TSS
- [x] Function to create a new process struct
  - [x] Create page directory
  - [x] Copy kernel tables
  - [x] Point malloc to here

## Physical Allocator

How to access region bitmask in paging mode?

I wrote the physical allocator before the paging allocator. So all operations
expect direct access (identity mapping) of the region bitmask page.

### Ideas

1. Have a dedicated virtual address for all possible region bitmask pages
   - Would require extra lookup from region table for physical addresses of each
     page (ie. cant just use bitmask address + page index)
2. ...

### Tasks

- [x] It looks like I might have already accounted for this in the virtual
  address space table. I need to check if this has been implemented.
  - The bitmasks are already mapped to their virtual space starting at 0x9f000
  - ram.h functions operate in virtual address space (except `ram_init` and `create_bitmask`)
- [x] Finish commenting `map_virt_page_dir` in kernel.c
