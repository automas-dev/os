#include "test_common.h"

DEFINE_FFF_GLOBALS;

void init_mocks() {
    FFF_RESET_HISTORY();

    reset_cpu_mmu_mock();
    reset_cpu_ports_mock();
    reset_cpu_tss_mock();
    reset_libc_datastruct_array_mock();
    reset_libc_memory_mock();
    reset_libc_proc_mock();
    reset_libc_string_mock();
    reset_libk_sys_call_mock();
    reset_log_mock();
    reset_ebus_mock();
    reset_memory_alloc_mock();
    reset_paging_mock();
    reset_process_mock();
    reset_ram_mock();
}
