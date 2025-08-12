#include "exec.h"

#include "cpu/mmu.h"
#include "cpu/tss.h"
#include "kernel.h"
#include "libc/memory.h"
#include "libc/proc.h"
#include "libc/stdio.h"
#include "libc/string.h"
#include "paging.h"
#include "process.h"
#include "ram.h"

typedef int (*ff_t)(size_t argc, char ** argv);

static void proc_entry();
static int  copy_args(process_t * proc, char * filepath, int argc, char ** argv);

int command_exec(uint8_t * buff, size_t size, size_t argc, char ** argv) {
    process_t * proc = kmalloc(sizeof(process_t));

    process_t * active = get_active_task();

    if (process_create(proc)) {
        puts("Failed to create process\n");
        return -1;
    }

    if (process_load_heap(proc, buff, size)) {
        puts("Failed to load\n");
        process_free(proc);
        return -1;
    }

    for (size_t i = 0; i < 1022; i++) {
        process_grow_stack(proc);
    }

    copy_args(proc, argv[0], argc, argv);

    process_set_entrypoint(proc, proc_entry);
    process_add_pages(proc, 32);
    pm_add_proc(kernel_get_proc_man(), proc);

    if (pm_resume_process(kernel_get_proc_man(), proc->pid, 0)) {
        return -1;
    }

    // pm_remove_proc(kernel_get_proc_man(), proc->pid);
    // process_free(proc);

    return proc->pid;
}

static void proc_entry() {
    process_t * proc = get_active_task();
    ff_t        fn   = UINT2PTR(VADDR_USER_MEM);

    // printf("Start task %s with %u args\n", proc->filepath, proc->argc);

    // TODO get start function pointer from elf

    int res           = fn(proc->argc, proc->argv);
    proc->status_code = res;
}

static char * copy_string(char * str) {
    int    len     = kstrlen(str);
    char * new_str = kmalloc(len + 1);
    kmemcpy(new_str, str, len + 1);
    return new_str;
}

static int copy_args(process_t * proc, char * filepath, int argc, char ** argv) {
    if (!proc || !filepath || !argv) {
        return -1;
    }

    proc->filepath = copy_string(filepath);
    proc->argc     = argc;
    proc->argv     = kmalloc(sizeof(char *) * argc);
    for (int i = 0; i < argc; i++) {
        proc->argv[i] = copy_string(argv[i]);
    }

    return 0;
}
