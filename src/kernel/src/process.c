#define KLOG_SERVICE "PROCESS"

#include "process.h"

#include "cpu/gdt.h"
#include "cpu/mmu.h"
#include "cpu/tss.h"
#include "drivers/ram.h"
#include "kernel.h"
#include "kernel/device/screen.h"
#include "kernel/logs.h"
#include "kernel/memory.h"
#include "libc/string.h"
#include "libk/sys_call.h"
#include "paging.h"

static int open_stdio_handles(process_t * proc);

static char * copy_string(const char * str);
static int    copy_to_process_pages(process_t * proc, uint32_t page_start, size_t count, const char * buff, size_t size);
static int    write_process_dwords(process_t * proc, uint32_t first_addr, const uint32_t * values, size_t count);

static uint32_t next_pid();
static uint32_t next_handle_id();

int process_create(process_t * proc) {
    if (!proc) {
        KLOG_WARNING("Process struct is null pointer");
        return -1;
    }

    if (!kmemset(proc, 0, sizeof(process_t))) {
        KLOG_ERROR("Failed to clear process struct");
        return -1;
    }

    proc->cr3 = ram_page_alloc();

    if (!proc->cr3) {
        KLOG_ERROR("Failed to allocate ram page for process page directory");
        return -1;
    }

    // Setup page directory
    mmu_dir_t * dir = paging_temp_map(proc->cr3);

    if (!dir) {
        KLOG_ERROR("Failed to create temporary mapping for physical address %p", proc->cr3);
        if (ram_page_free(proc->cr3)) {
            KLOG_DEBUG("Failed to free ram page %p", proc->cr3);
        }
        return -1;
    }

    uint32_t kernel_table_addr = mmu_dir_get_addr((mmu_dir_t *)VADDR_KERNEL_DIR, 0);
    if (!kernel_table_addr) {
        KLOG_ERROR("Failed to get physical address of kernel table");
        return -1;
    }

    // Copy first page table from kernel page directory
    mmu_dir_clear(dir);
    if (mmu_dir_set(dir, 0, kernel_table_addr, MMU_DIR_RW)) {
        KLOG_ERROR("Failed to set kernel table address in process page directory %p", dir);
        return -1;
    }

    proc->esp  = VADDR_ISR_STACK;
    proc->esp0 = VADDR_ISR_STACK;

    // Allocate the process' private ISR/kernel stack. This is supervisor-only
    // - ring 3 code must never read or write it directly. It is used both by
    // switch_task to save/restore this process' kernel-mode stack pointer, and
    // as the TSS esp0 target so a ring3->ring0 transition always starts fresh
    // here.
    uint32_t isr_stack_start = ADDR2PAGE(proc->esp0) - ISR_STACK_PAGES + 1;
    if (paging_add_pages(dir, isr_stack_start, ADDR2PAGE(proc->esp0), MMU_TABLE_RW)) {
        KLOG_DEBUG("Failed to create pages for isr stack");
        paging_temp_free(proc->cr3);
        ram_page_free(proc->cr3);
        return -1;
    }

    // Allocate the first page of the user (ring 3) stack. This must be
    // user-accessible so the process can use it once running in ring 3.
    if (paging_add_pages(dir, ADDR2PAGE(VADDR_USER_STACK), ADDR2PAGE(VADDR_USER_STACK), MMU_TABLE_RW_USER)) {
        KLOG_DEBUG("Failed to create page for user stack");
        paging_temp_free(proc->cr3);
        ram_page_free(proc->cr3);
        return -1;
    }

    proc->pid              = next_pid();
    proc->next_heap_page   = ADDR2PAGE(VADDR_USER_MEM);
    proc->stack_page_count = 1;

    // TODO parent process pid

    if (paging_temp_free(proc->cr3)) {
        KLOG_DEBUG("Failed to free temporary page after adding stack pages");
        ram_page_free(proc->cr3);
        return -1;
    }

    if (arr_create(&proc->io_handles, 4, sizeof(handle_t))) {
        KLOG_ERROR("Failed to create array for io handles");
        ram_page_free(proc->cr3);
        return -1;
    }

    // if (ebus_create(&proc->event_queue, 4096)) {
    //     KLOG_ERROR("Failed to create ebus for process event queue");
    //     arr_free(&proc->io_handles);
    //     ram_page_free(proc->cr3);
    //     return -1;
    // }

    proc->io_buffer = io_buffer_create(IO_BUFFER_SIZE);
    if (!proc->io_buffer) {
        KLOG_ERROR("Failed to create io buffer");
        // ebus_free(&proc->event_queue);
        arr_free(&proc->io_handles);
        ram_page_free(proc->cr3);
        return -1;
    }

    if (open_stdio_handles(proc)) {
        KLOG_DEBUG("Failed to open stdio handles");
        // ebus_free(&proc->event_queue);
        arr_free(&proc->io_handles);
        ram_page_free(proc->cr3);
        io_buffer_free(proc->io_buffer);
        return -1;
    }

    KLOG_INFO("Created process %u", proc->pid);

    return 0;
}

int process_free(process_t * proc) {
    if (!proc) {
        KLOG_WARNING("Process struct is null pointer");
        return -1;
    }

    // ebus_free(&proc->event_queue);
    arr_free(&proc->io_handles);

    mmu_dir_t * dir = paging_temp_map(proc->cr3);

    if (!dir) {
        KLOG_ERROR("Failed to create temporary mapping for process pageing directory");
        return -1;
    }

    // Free tables, skip first (kernel)
    for (size_t i = 1; i < MMU_DIR_SIZE; i++) {
        if (!(mmu_dir_get_flags(dir, i) & MMU_DIR_FLAG_PRESENT)) {
            continue;
        }

        uint32_t      table_addr = mmu_dir_get_addr(dir, i);
        mmu_table_t * table      = paging_temp_map(table_addr);

        if (!table) {
            KLOG_ERROR("Failed to create temporary mapping for process page table");
            paging_temp_free(proc->cr3);
            ram_page_free(proc->cr3);
            return -1;
        }

        // Free pages
        for (size_t j = 0; j < MMU_TABLE_SIZE; j++) {
            if (mmu_table_get_flags(table, j) & MMU_TABLE_FLAG_PRESENT) {
                uint32_t page_addr = mmu_table_get_addr(table, j);
                ram_page_free(page_addr);
            }
        }

        if (paging_temp_free(table_addr)) {
            KLOG_DEBUG("Failed to free temporary map of process page table");
            return -1;
        }
        if (ram_page_free(table_addr)) {
            KLOG_DEBUG("Failed to free ram page for process page table");
            return -1;
        }
    }

    // Free dir
    if (paging_temp_free(proc->cr3)) {
        KLOG_DEBUG("Failed to free temporary map of process page directory");
        return -1;
    }
    if (ram_page_free(proc->cr3)) {
        KLOG_DEBUG("Failed to free ram page for process page directory");
        return -1;
    }

    KLOG_INFO("Freed process %u", proc->pid);

    return 0;
}

/**
 * @brief Write `count` dwords into the process' own address space, starting
 * at `first_addr` (which must be dword-aligned) and going upward. `first_addr`
 * and the last dword written must fall within the same page.
 *
 * @param proc pointer to the process object
 * @param first_addr virtual address of the first dword to write
 * @param values dwords to write, in order
 * @param count number of dwords in values
 * @return int 0 for success
 */
static int write_process_dwords(process_t * proc, uint32_t first_addr, const uint32_t * values, size_t count) {
    uint32_t page_i  = ADDR2PAGE(first_addr);
    uint32_t dir_i   = page_i / MMU_TABLE_SIZE;
    uint32_t table_i = page_i % MMU_TABLE_SIZE;

    mmu_dir_t * dir = paging_temp_map(proc->cr3);

    if (!dir) {
        KLOG_ERROR("Failed to create temporary map for process page directory");
        return -1;
    }

    uint32_t table_addr = mmu_dir_get_addr(dir, dir_i);
    if (!table_addr) {
        KLOG_ERROR("Failed to get physical address of process page directory");
        paging_temp_free(proc->cr3);
        return -1;
    }

    mmu_table_t * table = paging_temp_map(table_addr);

    if (!table) {
        KLOG_ERROR("Failed to create temporary map for process page table");
        paging_temp_free(proc->cr3);
        return -1;
    }

    uint32_t page_addr = mmu_table_get_addr(table, table_i);
    if (!page_addr) {
        KLOG_ERROR("Failed to get physical address of process page");
        paging_temp_free(table_addr);
        paging_temp_free(proc->cr3);
        return -1;
    }

    uint32_t * page = paging_temp_map(page_addr);

    if (!page) {
        KLOG_ERROR("Failed to create temporary map for process page");
        paging_temp_free(table_addr);
        paging_temp_free(proc->cr3);
        return -1;
    }

    uint32_t first_i = (first_addr % PAGE_SIZE) / 4;
    for (size_t i = 0; i < count; i++) {
        page[first_i + i] = values[i];
    }

    if (paging_temp_free(page_addr)) {
        KLOG_DEBUG("Failed to free temporary map for process page");
        return -1;
    }
    if (paging_temp_free(table_addr)) {
        KLOG_DEBUG("Failed to free temporary map for process page table");
        return -1;
    }
    if (paging_temp_free(proc->cr3)) {
        KLOG_DEBUG("Failed to free temporary map for process page directory");
        return -1;
    }

    return 0;
}

int process_set_entrypoint(process_t * proc, void * entrypoint) {
    if (!proc) {
        KLOG_WARNING("Process struct is null pointer");
        return -1;
    }
    if (!entrypoint) {
        KLOG_WARNING("Entrypoint is null pointer");
        return -1;
    }
    if (proc->state >= PROCESS_STATE_SUSPENDED) {
        KLOG_WARNING("Process already started");
        return -1;
    }

    // Place argc/argv at the top of the ring 3 user stack so entry.asm's
    // __start can read them straight off the initial stack: [esp+0]=argc,
    // [esp+4]=argv (see process_copy_args for where argv itself is copied to
    // user-accessible memory).
    uint32_t args[2]  = {(uint32_t)proc->argc, PTR2UINT(proc->argv)};
    uint32_t user_esp = VADDR_USER_STACK - (sizeof(args) - 1);

    if (write_process_dwords(proc, user_esp, args, sizeof(args) / sizeof(args[0]))) {
        KLOG_DEBUG("Failed to write argc/argv onto process pid %u user stack", proc->pid);
        return -1;
    }

    // Frame written to the top of the process' private ISR/kernel stack so
    // that the first switch_task.resume for this process "returns" into
    // enter_usermode, which then `iret`s into ring 3 at `entrypoint`. The
    // first 4 dwords are popped as dummy registers by switch_task.resume; the
    // rest is the IRET frame the CPU expects (EIP, CS, EFLAGS, ESP, SS). See
    // kernel_entry.asm for the switch_task/enter_usermode contract.
    uint32_t frame[10] = {
        0,                        // dummy eax (popped by switch_task.resume)
        0,                        // dummy esi
        0,                        // dummy edi
        0,                        // dummy ebp
        PTR2UINT(enter_usermode), // "return address" for switch_task.resume
        PTR2UINT(entrypoint),     // iret: eip
        GDT_SELECTOR_USER_CODE,   // iret: cs
        0x202,                    // iret: eflags (IF set, reserved bit 1 set)
        user_esp,                 // iret: esp (top of ring 3 stack, past argc/argv)
        GDT_SELECTOR_USER_DATA,   // iret: ss
    };
    uint32_t frame_base = proc->esp0 - (sizeof(frame) - 1);

    if (write_process_dwords(proc, frame_base, frame, sizeof(frame) / sizeof(frame[0]))) {
        KLOG_DEBUG("Failed to write ring 3 launch frame for process pid %u", proc->pid);
        return -1;
    }

    proc->esp = frame_base;

    return 0;
}

int process_resume(process_t * proc, const ebus_event_t * event) {
    if (!proc) {
        KLOG_WARNING("Process struct is null pointer");
        return -1;
    }
    if (proc->state < PROCESS_STATE_LOADED) {
        KLOG_WARNING("Process not yet loaded");
        return -1;
    }
    if (proc->state >= PROCESS_STATE_DEAD) {
        KLOG_WARNING("Process is dead");
        return -1;
    }

    process_t * active_before = get_active_task();
    if (!active_before) {
        KPANIC("Failed to find active task");
    }

    if (active_before->state == PROCESS_STATE_RUNNING) {
        KLOG_TRACE("Setting state of active process %u to suspended", active_before->pid);
        active_before->state = PROCESS_STATE_SUSPENDED;
    }
    else {
        KLOG_TRACE("Skip updating active process %u status because it's not running, got %u", active_before->pid, active_before->state);
    }

    KLOG_TRACE("Setting process state for pid %u to SUSPENDED", active_before->pid);

    proc->state = PROCESS_STATE_RUNNING;
    KLOG_TRACE("Setting process state for pid %u to RUNNING", proc->pid);

    switch_task(proc);

    // Call this again because we are a new process now
    process_t * active_after = get_active_task();
    active_after->state      = PROCESS_STATE_RUNNING;

    return 0;
}

void * process_add_pages(process_t * proc, size_t count) {
    if (!proc) {
        KLOG_WARNING("Tried to add page to null process");
        return 0;
    }
    if (!count) {
        KLOG_WARNING("Add page count is 0");
        return 0;
    }

    if (proc->next_heap_page + count >= MMU_DIR_SIZE * MMU_TABLE_SIZE) {
        KLOG_WARNING("Cannot allocate %u pages after %u, will exceed max size of %d", count, proc->next_heap_page, MMU_DIR_SIZE * MMU_TABLE_SIZE);
        return 0;
    }

    mmu_dir_t * dir = paging_temp_map(proc->cr3);

    if (!dir) {
        KLOG_ERROR("Failed to map a temporary page for pid %u page dir", proc->pid);
        return 0;
    }

    if (paging_add_pages(dir, proc->next_heap_page, proc->next_heap_page + count, MMU_TABLE_RW_USER)) {
        KLOG_DEBUG("Failed to add %u pages to pid %u", count, proc->pid);
        paging_temp_free(proc->cr3);
        return 0;
    }

    if (paging_temp_free(proc->cr3)) {
        KLOG_DEBUG("Failed to free temporary page for process page directory");
        return 0;
    }

    void * ptr = UINT2PTR(PAGE2ADDR(proc->next_heap_page));
    proc->next_heap_page += count;

    return ptr;
}

int process_grow_stack(process_t * proc) {
    if (!proc) {
        KLOG_WARNING("Process struct is null pointer");
        return -1;
    }

    mmu_dir_t * dir = paging_temp_map(proc->cr3);

    if (!dir) {
        KLOG_ERROR("Failed to create temporary map of process page directory");
        return -1;
    }

    // Stack pages grow down starting immediately below the first user stack
    // page allocated by process_create (at ADDR2PAGE(VADDR_USER_STACK)).
    // proc->stack_page_count starts at 1 (that first page), so the Nth call
    // here (stack_page_count == N at this point) must land at
    // ADDR2PAGE(VADDR_USER_STACK) - N, not count down from the absolute top
    // of the address space - that would collide with (and silently convert
    // to user-accessible) the supervisor-only ISR stack pages above it.
    size_t new_stack_page_i = ADDR2PAGE(VADDR_USER_STACK) - proc->stack_page_count;

    if (paging_add_pages(dir, new_stack_page_i, new_stack_page_i, MMU_TABLE_RW_USER)) {
        KLOG_DEBUG("Failed to add pages for process stack");
        paging_temp_free(proc->cr3);
        return -1;
    }

    proc->stack_page_count++;

    if (paging_temp_free(proc->cr3)) {
        KLOG_DEBUG("Failed to free temporary map of process page dir");
        return -1;
    }

    return 0;
}

/**
 * @brief Copy `size` bytes from `buff` (kernel memory) into `count` pages of
 * the process' own address space, starting at page index `page_start`. The
 * destination pages must already be allocated (eg. via process_add_pages).
 *
 * @param proc pointer to the process object
 * @param page_start first destination page index
 * @param count number of pages to copy into
 * @param buff pointer to the source data (kernel memory)
 * @param size number of bytes to copy from buff
 * @return int 0 for success
 */
static int copy_to_process_pages(process_t * proc, uint32_t page_start, size_t count, const char * buff, size_t size) {
    mmu_dir_t * dir = paging_temp_map(proc->cr3);

    if (!dir) {
        KLOG_ERROR("Failed to create temporary mapping of process page directory");
        return -1;
    }

    for (size_t i = 0; i < count; i++) {
        uint32_t table_addr = mmu_dir_get_addr(dir, (page_start + i) / MMU_TABLE_SIZE);
        if (!table_addr) {
            KLOG_ERROR("Failed to get physical address of process pid %u page table", proc->pid);
            paging_temp_free(proc->cr3);
            return -1;
        }

        mmu_table_t * table = paging_temp_map(table_addr);
        if (!table) {
            KLOG_ERROR("Failed to create temporary mapping of process pid %u page table %p", proc->pid, table_addr);
            paging_temp_free(proc->cr3);
            return -1;
        }

        uint32_t addr = mmu_table_get_addr(table, (page_start + i) % MMU_TABLE_SIZE);
        if (!addr) {
            KLOG_ERROR("Failed to get physical address of process pid %u page", proc->pid);
            paging_temp_free(table_addr);
            paging_temp_free(proc->cr3);
            return -1;
        }

        void * tmp_page = paging_temp_map(addr);
        if (!tmp_page) {
            KLOG_ERROR("Failed to create temporary map of process pid %u page %p", proc->pid, addr);
            paging_temp_free(table_addr);
            paging_temp_free(proc->cr3);
            return -1;
        }

        size_t to_copy = PAGE_SIZE;

        if (i == count - 1) {
            // Bytes remaining for the last page. Using size % PAGE_SIZE here
            // would incorrectly copy 0 bytes (dropping the entire last page)
            // whenever size is an exact, nonzero multiple of PAGE_SIZE.
            to_copy = size - (i * PAGE_SIZE);
        }

        kmemcpy(tmp_page, &buff[i * PAGE_SIZE], to_copy);

        if (paging_temp_free(addr)) {
            KLOG_DEBUG("Failed to free temporary map of process pid %u page", proc->pid);
            return -1;
        }
        if (paging_temp_free(table_addr)) {
            KLOG_DEBUG("Failed to free temporary map of process pid %u page table", proc->pid);
            return -1;
        }
    }

    if (paging_temp_free(proc->cr3)) {
        KLOG_DEBUG("Failed to free temporary map of process pid %u page directory", proc->pid);
        return -1;
    }

    return 0;
}

void * process_copy_to_heap(process_t * proc, const void * buff, size_t size) {
    if (!proc) {
        KLOG_WARNING("Process struct is null pointer");
        return 0;
    }
    if (!buff) {
        KLOG_WARNING("Tried to copy null buffer to process heap");
        return 0;
    }
    if (!size) {
        KLOG_WARNING("Tried to copy 0 sized buffer to process heap");
        return 0;
    }

    size_t page_count = ADDR2PAGE(size);
    if (size & MASK_FLAGS) {
        KLOG_TRACE("Increasing page count by 1 to align with page boundary");
        page_count++;
    }

    uint32_t heap_start = proc->next_heap_page;
    void *   heap_addr  = process_add_pages(proc, page_count);

    if (!heap_addr) {
        KLOG_DEBUG("Failed to allocate pages for process pid %u", proc->pid);
        return 0;
    }

    if (copy_to_process_pages(proc, heap_start, page_count, buff, size)) {
        return 0;
    }

    return heap_addr;
}

int process_load_heap(process_t * proc, const char * buff, size_t size) {
    if (!proc) {
        KLOG_WARNING("Process struct is null pointer");
        return -1;
    }
    if (!buff) {
        KLOG_WARNING("Trying to load heap from null buffer");
        return -1;
    }
    if (!size) {
        KLOG_WARNING("Trying to load empty buffer");
        return -1;
    }

    KLOG_TRACE("Setting process pid %u state to LOADING", proc->pid);
    proc->state = PROCESS_STATE_LOADING;

    if (!process_copy_to_heap(proc, buff, size)) {
        KLOG_DEBUG("Failed to copy buffer into process pid %u heap", proc->pid);
        return -1;
    }

    KLOG_TRACE("Setting process pid %u state to LOADED", proc->pid);
    proc->state = PROCESS_STATE_LOADED;

    return 0;
}

static char * copy_string(const char * str) {
    if (!str) {
        KLOG_WARNING("Tried to copy null string");
        return 0;
    }
    size_t len     = kstrlen(str);
    char * new_str = kmalloc(len + 1);
    if (!new_str) {
        KLOG_ERROR("Failed to malloc new string of length %u", len + 1);
        return 0;
    }
    if (!kmemcpy(new_str, str, len + 1)) {
        KLOG_ERROR("Failed to copy %u bytes in memory from %p to %p", len + 1, str, new_str);
        return 0;
    }
    return new_str;
}

int process_copy_args(process_t * proc, const char * filepath, int argc, char ** argv) {
    if (!proc) {
        KLOG_WARNING("Tried to copy args for null process");
        return -1;
    }
    if (!filepath) {
        KLOG_WARNING("Missing filepath");
        return -1;
    }
    if (argc && !argv) {
        KLOG_WARNING("Missing argv");
        return -1;
    }

    // filepath is kernel bookkeeping only (eg. logging), never handed to ring
    // 3 code, so it can stay in kernel memory.
    proc->filepath = copy_string(filepath);
    if (!proc->filepath) {
        KLOG_DEBUG("Failed to copy filepath");
        return -1;
    }

    // argv (the pointer array and the strings themselves) is the process'
    // real argument list and must live in its own user-accessible memory, not
    // kernel memory, so ring 3 code can safely read it. Build the whole thing
    // (array + strings) in a kernel scratch buffer first, with pointer values
    // already rewritten to where the data will land in the process' heap,
    // then copy the scratch buffer in one go.
    size_t ptr_bytes = (argc + 1) * sizeof(char *);
    size_t str_bytes = kstrlen(filepath) + 1;

    for (int i = 0; i < argc; i++) {
        str_bytes += kstrlen(argv[i]) + 1;
    }

    size_t total_bytes = ptr_bytes + str_bytes;

    char * scratch = kmalloc(total_bytes);
    if (!scratch) {
        KLOG_ERROR("Failed to malloc scratch buffer of size %u for process pid %u args", total_bytes, proc->pid);
        kfree(proc->filepath);
        return -1;
    }

    size_t page_count = ADDR2PAGE(total_bytes);
    if (total_bytes & MASK_FLAGS) {
        page_count++;
    }

    uint32_t heap_start = proc->next_heap_page;
    void *   heap_addr  = process_add_pages(proc, page_count);

    if (!heap_addr) {
        KLOG_DEBUG("Failed to allocate pages for process pid %u args", proc->pid);
        kfree(scratch);
        kfree(proc->filepath);
        return -1;
    }

    char **  argv_ptrs = (char **)scratch;
    char *   str_dest  = scratch + ptr_bytes;
    uint32_t str_addr  = PTR2UINT(heap_addr) + ptr_bytes;
    size_t   len;

    // argv[0] is always the program filename
    len = kstrlen(filepath) + 1;
    kmemcpy(str_dest, filepath, len);
    argv_ptrs[0] = UINT2PTR(str_addr);
    str_dest += len;
    str_addr += len;

    for (int i = 0; i < argc; i++) {
        len = kstrlen(argv[i]) + 1;
        kmemcpy(str_dest, argv[i], len);
        argv_ptrs[i + 1] = UINT2PTR(str_addr);
        str_dest += len;
        str_addr += len;
    }

    if (copy_to_process_pages(proc, heap_start, page_count, scratch, total_bytes)) {
        KLOG_DEBUG("Failed to copy args into process pid %u heap", proc->pid);
        kfree(scratch);
        kfree(proc->filepath);
        return -1;
    }

    kfree(scratch);

    proc->argc = argc + 1;
    proc->argv = heap_addr;

    return 0;
}

handle_t * process_get_handle(process_t * proc, int id) {
    if (!proc) {
        KLOG_WARNING("Process struct is null pointer");
        return 0;
    }
    if (id < 0) {
        KLOG_WARNING("id must be >= 0, got %d", id);
        return 0;
    }

    for (size_t i = 0; i < arr_size(&proc->io_handles); i++) {
        handle_t * h = arr_at(&proc->io_handles, i);

        if (h->id == id) {
            return h;
        }
    }

    KLOG_WARNING("Failed to find handle %d for process pid %u", id, proc->pid);

    return 0;
}

int process_add_handle(process_t * proc, int id, int flags, io_device_t * device) {
    if (!proc) {
        KLOG_WARNING("Process struct is null pointer");
        return -1;
    }
    if (!device) {
        KLOG_WARNING("Devices is a null pointer");
        return -1;
    }

    if (id < 0) {
        id = next_handle_id();
        KLOG_DEBUG("Generated handle id %d", id);
    }
    else {
        KLOG_DEBUG("Using provided handle id %d", id);
    }

    handle_t h = {
        .id     = id,
        .flags  = flags,
        .device = device,
    };

    if (arr_insert(&proc->io_handles, arr_size(&proc->io_handles), &h)) {
        KLOG_ERROR("Could not add new handle to process %u", proc->pid);
        return -1;
    }

    KLOG_DEBUG("Added handle %d to process %u with flags 0x%X", id, proc->pid, flags);

    return id;
}

static int open_stdio_handles(process_t * proc) {
    if (!proc) {
        KLOG_WARNING("Process struct is null pointer");
        return -1;
    }

    // TODO make stdin

    // process_add_handle returns handle id
    if (process_add_handle(proc, 1, IO_DEVICE_FLAG_WRITE, io_device_screen_open()) < 0) {
        KLOG_DEBUG("Failed to create stdout handle");
        return -1;
    }

    KLOG_DEBUG("Created stdout handle 1 for process %u", proc->pid);

    if (process_add_handle(proc, 2, IO_DEVICE_FLAG_WRITE, io_device_screen_open()) < 0) {
        KLOG_DEBUG("Failed to create stderr handle");
        return -1;
    }

    KLOG_DEBUG("Created stderr handle 2 for process %u", proc->pid);

    set_next_handle_id(3);

    // TODO should this be debug or trace
    KLOG_DEBUG("Next handle set to 3");

    return 0;
}

int process_link(process_t * proc, process_t * next) {
    if (!proc) {
        KLOG_WARNING("Process struct is null pointer");
        return -1;
    }
    if (!next) {
        KLOG_WARNING("Next process struct is null pointer");
        return -1;
    }
    if (proc->next && proc->next->prev != proc) {
        KLOG_ERROR("proc has a bad link, next does not link back to proc");
        return -1;
    }

    next->prev = proc;
    next->next = proc->next;

    proc->next->prev = next;
    proc->next       = next;

    return 0;
}

int process_unlink(process_t * proc) {
    if (!proc) {
        KLOG_WARNING("Process struct is null pointer");
        return -1;
    }
    if (!proc->prev || !proc->next) {
        KLOG_WARNING("Process struct is not linked");
        return -1;
    }

    proc->prev->next = proc->next;
    proc->next->prev = proc->prev;

    proc->next = 0;
    proc->prev = 0;

    return 0;
}

static uint32_t __pid;

static uint32_t next_pid() {
    static int pid_set = 0;
    // Handle initializing __pid because there is no static init
    if (!pid_set) {
        __pid   = 1;
        pid_set = 1;
    }
    return __pid++;
}

void set_next_pid(uint32_t next) {
    next_pid(); // Force pid_set to true so it doesn't override this value
    __pid = next;
}

static uint32_t __handle_id;

static uint32_t next_handle_id() {
    static int handle_id_set = 0;
    // Handle initializing __pid because there is no static init
    if (!handle_id_set) {
        __handle_id   = 3;
        handle_id_set = 1;
    }
    return __handle_id++;
}

void set_next_handle_id(uint32_t next) {
    next_handle_id(); // Force handle_id_set to true so it doesn't override this value
    __handle_id = next;
}
