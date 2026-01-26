#include "process.h"

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

#undef SERVICE
#define SERVICE "KERNEL/PROCESS"

static int open_stdio_handles(process_t * proc);

static uint32_t next_pid();
static uint32_t next_handle_id();

int process_create(process_t * proc) {
    if (!proc) {
        KLOG_ERROR("Process struct is null pointer");
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
            KLOG_ERROR("Failed to free ram page %p for process page directory", proc->cr3);
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

    proc->esp  = VADDR_USER_STACK;
    proc->esp0 = VADDR_ISR_STACK;

    // Allocate pages for ISR stack + first page of user stack
    if (paging_add_pages(dir, ADDR2PAGE(proc->esp), ADDR2PAGE(proc->esp0))) {
        KLOG_ERROR("Failed to create pages for isr and user stacks");
        // Error logged in function, not logging here because we are already returning error
        paging_temp_free(proc->cr3);
        ram_page_free(proc->cr3);
        return -1;
    }

    proc->pid              = next_pid();
    proc->next_heap_page   = ADDR2PAGE(VADDR_USER_MEM);
    proc->stack_page_count = 1;

    if (paging_temp_free(proc->cr3)) {
        KLOG_ERROR("Failed to free temporary page");
        ram_page_free(proc->cr3);
        return -1;
    }

    if (arr_create(&proc->io_handles, 4, sizeof(handle_t))) {
        KLOG_ERROR("Failed to create array for io handles");
        ram_page_free(proc->cr3);
        return -1;
    }

    if (ebus_create(&proc->event_queue, 4096)) {
        KLOG_ERROR("Failed to create ebus for process event queue");
        arr_free(&proc->io_handles);
        ram_page_free(proc->cr3);
        return -1;
    }

    if (memory_init(&proc->memory, kernel_alloc_page)) {
        KLOG_ERROR("Failed to initialize malloc for process");
        ebus_free(&proc->event_queue);
        arr_free(&proc->io_handles);
        ram_page_free(proc->cr3);
        return -1;
    }

    proc->io_buffer = io_buffer_create(IO_BUFFER_SIZE);
    if (!proc->io_buffer) {
        KLOG_ERROR("Failed to create io buffer");
        ebus_free(&proc->event_queue);
        arr_free(&proc->io_handles);
        ram_page_free(proc->cr3);
        return -1;
    }

    if (open_stdio_handles(proc)) {
        KLOG_ERROR("Failed to open stdio handles");
        ebus_free(&proc->event_queue);
        arr_free(&proc->io_handles);
        ram_page_free(proc->cr3);
        io_buffer_free(proc->io_buffer);
        return -1;
    }

    return 0;
}

int process_free(process_t * proc) {
    if (!proc) {
        KLOG_ERROR("Process struct is null pointer");
        return -1;
    }

    ebus_free(&proc->event_queue);
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
            KLOG_ERROR("Failed to free temporary map of process page table");
            return -1;
        }
        if (ram_page_free(table_addr)) {
            KLOG_ERROR("Failed to free ram page for process page table");
            return -1;
        }
    }

    // Free dir
    if (paging_temp_free(proc->cr3)) {
        KLOG_ERROR("Failed to free temporary map of process page directory");
        return -1;
    }
    if (ram_page_free(proc->cr3)) {
        KLOG_ERROR("Failed to free ram page for process page directory");
        return -1;
    }

    return 0;
}

int process_set_entrypoint(process_t * proc, void * entrypoint) {
    if (!proc) {
        KLOG_ERROR("Process struct is null pointer");
        return -1;
    }
    if (!entrypoint) {
        KLOG_ERROR("Entrypoint is null pointer");
        return -1;
    }
    if (proc->state >= PROCESS_STATE_SUSPENDED) {
        KLOG_ERROR("Process already started");
        return -1;
    }

    uint32_t ret_addr = proc->esp;
    uint32_t ret_page = ADDR2PAGE(ret_addr);
    uint32_t dir_i    = ret_page / MMU_DIR_SIZE;
    uint32_t table_i  = ret_page % MMU_TABLE_SIZE;

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
    if (!table_addr) {
        KLOG_ERROR("Failed to get physical address of process page");
        paging_temp_free(table_addr);
        paging_temp_free(proc->cr3);
        return -1;
    }

    uint32_t * stack = paging_temp_map(page_addr);

    if (!stack) {
        KLOG_ERROR("Failed to create temporary map for process stack page");
        paging_temp_free(table_addr);
        paging_temp_free(proc->cr3);
        return -1;
    }

    int ret_i    = (proc->esp % PAGE_SIZE) / 4;
    stack[ret_i] = PTR2UINT(entrypoint);

    if (paging_temp_free(page_addr)) {
        KLOG_ERROR("Failed to free temporary map for process stack page");
        return -1;
    }
    if (paging_temp_free(table_addr)) {
        KLOG_ERROR("Failed to free temporary map for process page table");
        return -1;
    }
    if (paging_temp_free(proc->cr3)) {
        KLOG_ERROR("Failed to free temporary map for process page directory");
        return -1;
    }

    proc->esp -= (5 * 4) - 1;

    return 0;
}

int process_resume(process_t * proc, const ebus_event_t * event) {
    if (!proc) {
        KLOG_ERROR("Process struct is null pointer");
        return -1;
    }
    if (proc->state < PROCESS_STATE_LOADED) {
        KLOG_ERROR("Process not yet started");
        return -1;
    }
    if (proc->state >= PROCESS_STATE_DEAD) {
        KLOG_ERROR("Process is dead");
        return -1;
    }

    process_t * active_before = get_active_task();
    if (!active_before) {
        KPANIC("Failed to find active task");
    }
    active_before->state = PROCESS_STATE_SUSPENDED;

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
        KLOG_ERROR("Tried to add page to null process");
        return 0;
    }
    if (!count) {
        KLOG_WARNING("Add page count is 0");
        return 0;
    }

    if (proc->next_heap_page + count >= MMU_DIR_SIZE * MMU_TABLE_SIZE) {
        KLOG_ERROR("Cannot allocate %u pages after %u, will exceed max size of %d", count, proc->next_heap_page, MMU_DIR_SIZE * MMU_TABLE_SIZE);
        return 0;
    }

    mmu_dir_t * dir = paging_temp_map(proc->cr3);

    if (!dir) {
        KLOG_ERROR("Failed to map a temporary page for pid %u page dir", proc->pid);
        return 0;
    }

    if (paging_add_pages(dir, proc->next_heap_page, proc->next_heap_page + count)) {
        KLOG_ERROR("Failed to add %u pages to pid %u", count, proc->pid);
        paging_temp_free(proc->cr3);
        return 0;
    }

    if (paging_temp_free(proc->cr3)) {
        KLOG_ERROR("Failed to free temporary page for process page directory");
        return 0;
    }

    void * ptr = UINT2PTR(PAGE2ADDR(proc->next_heap_page));
    proc->next_heap_page += count;

    return ptr;
}

int process_grow_stack(process_t * proc) {
    if (!proc) {
        KLOG_ERROR("Process struct is null pointer");
        return -1;
    }

    mmu_dir_t * dir = paging_temp_map(proc->cr3);

    if (!dir) {
        KLOG_ERROR("Failed to create temporary map of process page directory");
        return -1;
    }

    size_t new_stack_page_i = MMU_DIR_SIZE * MMU_TABLE_SIZE - proc->stack_page_count - 1;

    if (paging_add_pages(dir, new_stack_page_i, new_stack_page_i)) {
        KLOG_ERROR("Failed to add pages fo process stack");
        paging_temp_free(proc->cr3);
        return -1;
    }

    proc->stack_page_count++;

    if (paging_temp_free(proc->cr3)) {
        KLOG_ERROR("Failed to free temporary map of process page dir");
        return -1;
    }

    return 0;
}

int process_load_heap(process_t * proc, const char * buff, size_t size) {
    if (!proc) {
        KLOG_ERROR("Process struct is null pointer");
        return -1;
    }
    if (!buff) {
        KLOG_ERROR("Trying to load heap from null buffer");
        return -1;
    }
    if (!size) {
        KLOG_ERROR("Trying to load empty buffer");
        return -1;
    }

    KLOG_TRACE("Setting process pid %u state to LOADING", proc->pid);
    proc->state = PROCESS_STATE_LOADING;

    size_t page_count = ADDR2PAGE(size);
    if (size & MASK_FLAGS) {
        KLOG_TRACE("Increasing page count by 1 to align with page boundary");
        page_count++;
    }

    uint32_t heap_start = proc->next_heap_page;
    void *   heap_alloc = process_add_pages(proc, page_count);

    if (!heap_alloc) {
        KLOG_ERROR("Failed to allocate pages for process pid %u heap", proc->pid);
        return -1;
    }

    mmu_dir_t * dir = paging_temp_map(proc->cr3);

    if (!dir) {
        KLOG_ERROR("Failed to create temporary mapping of process page directory");
        return -1;
    }

    for (size_t i = 0; i < page_count; i++) {
        uint32_t table_addr = mmu_dir_get_addr(dir, (heap_start + i) / MMU_TABLE_SIZE);
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

        uint32_t addr = mmu_table_get_addr(table, (heap_start + i) % MMU_TABLE_SIZE);
        if (!table_addr) {
            KLOG_ERROR("Failed to get physical address of process pid %u page table", proc->pid);
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

        if (i == page_count - 1) {
            to_copy = size % PAGE_SIZE;
        }

        kmemcpy(tmp_page, &buff[i * PAGE_SIZE], to_copy);

        if (paging_temp_free(addr)) {
            KLOG_ERROR("Failed to free temporary map of process pid %u page");
            return -1;
        }
        if (paging_temp_free(table_addr)) {
            KLOG_ERROR("Failed to free temporary map of process pid %u page table");
            return -1;
        }
    }

    if (paging_temp_free(proc->cr3)) {
        KLOG_ERROR("Failed to free temporary map of process pid %u page directory");
        return -1;
    }

    KLOG_TRACE("Setting process pid %u state to LOADED", proc->pid);
    proc->state = PROCESS_STATE_LOADED;

    return 0;
}

handle_t * process_get_handle(process_t * proc, int id) {
    if (!proc) {
        KLOG_ERROR("Process struct is null pointer");
        return 0;
    }
    if (id < 0) {
        KLOG_ERROR("id must be >= 0, got %d", id);
        return 0;
    }

    for (size_t i = 0; i < arr_size(&proc->io_handles); i++) {
        handle_t * h = arr_at(&proc->io_handles, i);

        if (h->id == id) {
            return h;
        }
    }

    KLOG_ERROR("Failed to find handle %d for process pid %u", id, proc->pid);

    return 0;
}

int process_add_handle(process_t * proc, int id, int flags, io_device_t * device) {
    if (!proc) {
        KLOG_ERROR("Process struct is null pointer");
        return -1;
    }
    if (!device) {
        KLOG_ERROR("Devices is a null pointer");
        return -1;
    }

    if (id < 0) {
        id = next_handle_id();
    }

    handle_t h = {
        .id     = id,
        .flags  = flags,
        .device = device,
    };

    if (arr_insert(&proc->io_handles, arr_size(&proc->io_handles), &h)) {
        KLOG_ERROR("Could not add new handle to process");
        return -1;
    }

    return id;
}

static int open_stdio_handles(process_t * proc) {
    if (!proc) {
        KLOG_ERROR("Process struct is null pointer");
        return -1;
    }

    // TODO make stdin

    // process_add_handle returns handle id
    if (process_add_handle(proc, 1, DEVICE_IO_FLAG_WRITE, device_screen_open()) < 0) {
        KLOG_ERROR("Failed to create stdout handle");
        return -1;
    }

    handle_t * h = arr_at(&proc->io_handles, 0);

    if (process_add_handle(proc, 2, DEVICE_IO_FLAG_WRITE, device_screen_open()) < 0) {
        KLOG_ERROR("Failed to create stderr handle");
        return -1;
    }

    handle_t * h2 = arr_at(&proc->io_handles, 1);

    set_next_handle_id(3);

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
