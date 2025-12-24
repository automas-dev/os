#ifndef KERNEL_PROCESS_H
#define KERNEL_PROCESS_H

#include <stddef.h>
#include <stdint.h>

#include "ebus.h"
#include "kernel/device/io.h"
#include "kernel/io_buffer.h"
#include "libc/datastruct/array.h"
#include "libc/file.h"
#include "memory_alloc.h"

#define IO_BUFFER_SIZE 512

typedef void (*signals_master_cb_t)(int);

// enum HANDLE_TYPE {
//     HANDLE_TYPE_FREE = 0,
//     HANDLE_TYPE_FILE,
//     HANDLE_TYPE_DIR,
// };

typedef struct _handle {
    int id;
    int flags;
    // int type;

    io_device_t * device;
} handle_t;

enum PROCESS_STATE {
    PROCESS_STATE_STARTING = 0,
    PROCESS_STATE_LOADING,
    PROCESS_STATE_LOADED,
    PROCESS_STATE_SUSPENDED,
    PROCESS_STATE_WAITING,
    // PROCESS_STATE_WAITING_STDIN,
    PROCESS_STATE_RUNNING,
    PROCESS_STATE_DEAD,
    PROCESS_STATE_ERROR,
};

typedef struct _process {
    uint32_t cr3;
    uint32_t esp;
    uint32_t esp0;

    uint32_t pid;
    uint32_t next_heap_page;
    uint32_t stack_page_count;

    // TODO heap & stack limits

    char *  filepath;
    int     argc;
    char ** argv;
    int     status_code;

    signals_master_cb_t signals_callback;
    arr_t               io_handles; // array<handle_t>
    ebus_t              event_queue;
    memory_t            memory;

    uint32_t           filter_event;
    enum PROCESS_STATE state;

    io_buffer_t * io_buffer;
} process_t;

/**
 * @brief Create a new process and it's page directory.
 *
 * Allocates pages for the isr stack and 1 for the user stack.
 *
 * @param proc pointer to the process object
 * @return int 0 for success
 */
int process_create(process_t * proc);

/**
 * @brief Free pages used by `process` including it's page directory.
 *
 * This does not free the first table which is the kernel's table.
 *
 * @param proc pointer to the process object
 * @return int 0 for success
 */
int process_free(process_t * proc);

/**
 * @brief Set the entry point or eip of the process.
 *
 * This entrypoint is used when the process starts or is resumed.
 *
 * @param proc pointer to the process object
 * @param entrypoint eip or address of the entrypoint or function
 * @return int 0 for success
 */
int process_set_entrypoint(process_t * proc, void * entrypoint);

/**
 * @brief Activate and jump to the process.
 *
 * If the jump is successful this function will never return. Returning from
 * this function signals an error. If this is the first call, the entrypoint is
 * used. If this is resuming from a yield, the last eip will be used.
 *
 * @param proc pointer to the process object
 * @param event ebus event to return from yield or 0
 * @return int No Return, if this function returns it is an error
 */
int process_resume(process_t * proc, const ebus_event_t * event);

/**
 * @brief Add `count` pages to the process heap.
 *
 * @param proc pointer to the process object
 * @param count number of pages to add
 * @return pointer to the first new page in virtual memory
 */
void * process_add_pages(process_t * proc, size_t count);

/**
 * @brief Add a single page to expand the process stack
 *
 * @param proc pointer to the process object
 * @return int 0 for success
 */
int process_grow_stack(process_t * proc);

/**
 * @brief Allocate pages in the heap and copy data from `buff` into the pages.
 *
 * Pages are allocated using process_add_pages. If process_add_pages has not not
 * yet been called, this will place the data at the start of the user memory.
 * This is ideal for loading a process executable, as it has a fixed / known
 * memory address to start execution.
 *
 * If process_add_pages or process_load_heap have been called, this function
 * will begin allocating pages starting with the end of the heap. This is page
 * aligned, so if the size of the last call process_load_heap was not page
 * aligned, there will be a gap between the end of the last data and this new
 * data.
 *
 * @param proc pointer to the process object
 * @param buff pointer to the data
 * @param size number of bytes to copy from buff
 * @return int 0 for success
 */
int process_load_heap(process_t * proc, const char * buff, size_t size);

int process_add_handle(process_t * proc, int id, int flags, io_device_t * device);

handle_t * process_get_handle(process_t * proc, int id);

/**
 * @brief Set the next PID value. All future PID's will be incremented from
 * here.
 *
 * This can be used to reset, replace os skip certain PID values.
 *
 * @param next process id
 */
void set_next_pid(uint32_t next);

void set_next_handle_id(uint32_t next);

extern void        set_active_task(process_t * active);
extern process_t * get_active_task(void);
extern void        switch_task(process_t * proc);
extern void        start_first_task(process_t * proc);

#endif // KERNEL_PROCESS_H
