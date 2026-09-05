#include <array>
#include <cstdlib>

#include "test_common.h"

extern "C" {
#include "addr.h"
#include "cpu/gdt.h"
#include "cpu/mmu.h"
#include "libc/datastruct/array.h"
#include "process.h"

mmu_dir_t   dir;
mmu_table_t table;
process_t   proc;
process_t   alt_proc;
io_buffer_t io_buffer;
io_device_t io_device;

int custom_mmu_dir_set(mmu_dir_t * dir, size_t i, uint32_t addr, uint32_t flags) {
    if (!dir || i >= MMU_DIR_SIZE) {
        return -1;
    }

    mmu_entry_t entry = (addr & MASK_ADDR) | (flags & MASK_FLAGS);

    dir->entries[i] = entry;

    return 0;
}

int custom_mmu_table_set(mmu_table_t * table, size_t i, uint32_t addr, uint32_t flags) {
    if (!table || i >= MMU_DIR_SIZE) {
        return -1;
    }

    mmu_entry_t entry = (addr & MASK_ADDR) | (flags & MASK_FLAGS);

    table->entries[i] = entry;

    return 0;
}
}

std::array<char, PAGE_SIZE * 3>                 temp_page;
std::array<char, PAGE_SIZE * 2 + PAGE_SIZE / 2> heap_data;

class Process : public ::testing::Test {
protected:
    void SetUp() override {
        init_mocks();

        memset(&dir, 0, sizeof(dir));
        memset(&table, 0, sizeof(table));
        memset(&proc, 0, sizeof(proc));
        memset(&alt_proc, 0, sizeof(alt_proc));
        memset(&io_buffer, 0, sizeof(io_buffer));
        memset(&io_device, 0, sizeof(io_device));

        temp_page.fill(0);

        for (size_t i = 0; i < heap_data.size(); i++) {
            heap_data[i] = i % 0xff;
        }

        proc.next_heap_page = 2;

        mmu_dir_set_fake.custom_fake       = custom_mmu_dir_set;
        mmu_table_set_fake.custom_fake     = custom_mmu_table_set;
        mmu_dir_get_addr_fake.return_val   = 0x5000;
        mmu_table_get_addr_fake.return_val = 0x6000;

        paging_temp_map_fake.return_val       = temp_page.data();
        io_buffer_create_fake.return_val      = &io_buffer;
        io_device_screen_open_fake.return_val = &io_device;
    }
};

// Process Create

TEST_F(Process, process_create_InvalidParameters) {
    EXPECT_NE(0, process_create(0));
}

TEST_F(Process, process_create_FailDirAlloc) {
    ram_page_alloc_fake.return_val = 0;

    EXPECT_NE(0, process_create(&proc));
    ASSERT_RAM_ALLOC_BALANCE_OFFSET(1);
}

TEST_F(Process, process_create_FailCreateArray) {
    ram_page_alloc_fake.return_val = 0x2000;
    arr_create_fake.return_val     = -1;

    EXPECT_NE(0, process_create(&proc));
    EXPECT_EQ(1, ram_page_free_fake.call_count);
    ASSERT_RAM_ALLOC_BALANCED();
}

TEST_F(Process, process_create_EbusIsUnused) {
    ram_page_alloc_fake.return_val = 0x2000;
    ebus_create_fake.return_val    = -1;

    EXPECT_EQ(0, process_create(&proc));
    EXPECT_EQ(0, ebus_create_fake.call_count);
}

TEST_F(Process, process_create_MemoryIsUnused) {
    // proc->memory (this process' own malloc) is intentionally NOT
    // initialized by process_create - see process_init_memory. Only the
    // kernel's own kmalloc pool (not exercised by process_create at all) is
    // available while this process is being created; memory_init requires
    // the process' own cr3 to be loaded, which it isn't yet.
    ram_page_alloc_fake.return_val = 0x2000;
    memory_init_fake.return_val    = -1;

    EXPECT_EQ(0, process_create(&proc));
    EXPECT_EQ(0, memory_init_fake.call_count);
}

TEST_F(Process, process_create_FailDirTempMap) {
    uint32_t page_ret_seq[2] = {0x400000, 0};
    SET_RETURN_SEQ(ram_page_alloc, page_ret_seq, 2);

    paging_temp_map_fake.return_val = 0;

    EXPECT_NE(0, process_create(&proc));
    EXPECT_EQ(1, ram_page_free_fake.call_count);
    ASSERT_TEMP_MAP_BALANCE_OFFSET(1);
    ASSERT_RAM_ALLOC_BALANCED();
}

TEST_F(Process, process_create_FailAddPages) {
    ram_page_alloc_fake.return_val   = 0x2000;
    paging_temp_map_fake.return_val  = &dir;
    paging_add_pages_fake.return_val = -1;

    EXPECT_NE(0, process_create(&proc));
    EXPECT_EQ(1, paging_add_pages_fake.call_count);
    EXPECT_EQ(1, paging_temp_free_fake.call_count);
    EXPECT_EQ(1, ram_page_free_fake.call_count);
    ASSERT_TEMP_MAP_BALANCED();
    ASSERT_RAM_ALLOC_BALANCED();
}

TEST_F(Process, process_create_FailAddUserStackPage) {
    ram_page_alloc_fake.return_val  = 0x2000;
    paging_temp_map_fake.return_val = &dir;

    // ISR stack pages succeed, first user stack page fails
    int add_pages_seq[2] = {0, -1};
    SET_RETURN_SEQ(paging_add_pages, add_pages_seq, 2);

    EXPECT_NE(0, process_create(&proc));
    EXPECT_EQ(2, paging_add_pages_fake.call_count);
    EXPECT_EQ(1, paging_temp_free_fake.call_count);
    EXPECT_EQ(1, ram_page_free_fake.call_count);
    ASSERT_TEMP_MAP_BALANCED();
    ASSERT_RAM_ALLOC_BALANCED();
}

TEST_F(Process, process_create) {
    ram_page_alloc_fake.return_val   = 0x2000; // physical page for dir
    paging_temp_map_fake.return_val  = &dir;   // dir temp mapped to virtual
    mmu_dir_get_addr_fake.return_val = 0x5000; // table physical addr
    set_next_pid(12);

    EXPECT_EQ(0, process_create(&proc));

    // Fields are set
    EXPECT_EQ(0x2000, proc.cr3);
    EXPECT_EQ(12, proc.pid);
    EXPECT_EQ(1024, proc.next_heap_page);
    EXPECT_EQ(1, proc.stack_page_count);
    EXPECT_EQ(0xffffffff, proc.esp);
    EXPECT_EQ(0xffffffff, proc.esp0);

    // Dir has correct contents
    EXPECT_EQ(0x5003, dir.entries[0]);
    for (size_t i = 1; i < 1024; i++) {
        EXPECT_EQ(0, dir.entries[i]);
    }

    EXPECT_EQ(2, paging_add_pages_fake.call_count);

    // ISR / kernel stack: supervisor-only
    EXPECT_EQ(&dir, paging_add_pages_fake.arg0_history[0]);
    EXPECT_EQ(0xffff0, paging_add_pages_fake.arg1_history[0]);
    EXPECT_EQ(0xfffff, paging_add_pages_fake.arg2_history[0]);
    EXPECT_EQ((uint32_t)MMU_TABLE_RW, paging_add_pages_fake.arg3_history[0]);

    // First user stack page: user-accessible
    EXPECT_EQ(&dir, paging_add_pages_fake.arg0_history[1]);
    EXPECT_EQ(0xfffef, paging_add_pages_fake.arg1_history[1]);
    EXPECT_EQ(0xfffef, paging_add_pages_fake.arg2_history[1]);
    EXPECT_EQ((uint32_t)MMU_TABLE_RW_USER, paging_add_pages_fake.arg3_history[1]);

    ASSERT_TEMP_MAP_BALANCED();
    ASSERT_RAM_ALLOC_BALANCE_OFFSET(1);
}

// Process Free

TEST_F(Process, process_free_InvalidParameters) {
    EXPECT_NE(0, process_free(0));
}

TEST_F(Process, process_free_FailTempMap) {
    paging_temp_map_fake.return_val = 0;

    EXPECT_NE(0, process_free(&proc));
    ASSERT_TEMP_MAP_BALANCE_OFFSET(1);
}

TEST_F(Process, process_free_FailSecondTempMap) {
    void * paging_temp_map_seq[2] = {&dir, 0};
    SET_RETURN_SEQ(paging_temp_map, paging_temp_map_seq, 2);

    mmu_dir_get_flags_fake.return_val = MMU_DIR_FLAG_PRESENT;

    EXPECT_NE(0, process_free(&proc));
    EXPECT_EQ(1, mmu_dir_get_flags_fake.call_count);
    EXPECT_EQ(1, mmu_dir_get_addr_fake.call_count);
    EXPECT_EQ(2, paging_temp_map_fake.call_count);
    EXPECT_EQ(1, paging_temp_free_fake.call_count);
    EXPECT_EQ(1, ram_page_free_fake.call_count);
    ASSERT_TEMP_MAP_BALANCE_OFFSET(1);
}

TEST_F(Process, process_free_NoPages) {
    paging_temp_map_fake.return_val   = &dir;
    mmu_dir_get_flags_fake.return_val = MMU_DIR_FLAG_PRESENT;

    EXPECT_EQ(0, process_free(&proc));
    EXPECT_EQ(MMU_DIR_SIZE, ram_page_free_fake.call_count); // tables + dir
    ASSERT_TEMP_MAP_BALANCED();
}

TEST_F(Process, process_free_NoTables) {
    paging_temp_map_fake.return_val = &dir;

    EXPECT_EQ(0, process_free(&proc));
    EXPECT_EQ(1, ram_page_free_fake.call_count);
    EXPECT_EQ(1, arr_free_fake.call_count);
    ASSERT_TEMP_MAP_BALANCED();
}

TEST_F(Process, process_free) {
    paging_temp_map_fake.return_val     = &dir;
    mmu_dir_get_flags_fake.return_val   = MMU_DIR_FLAG_PRESENT;
    mmu_table_get_flags_fake.return_val = MMU_TABLE_FLAG_PRESENT;

    int page_count  = (MMU_DIR_SIZE - 1) * MMU_TABLE_SIZE;
    int table_count = MMU_DIR_SIZE - 1;
    int dir_count   = 1;

    int expect_free_count = page_count + table_count + dir_count;

    EXPECT_EQ(0, process_free(&proc));
    EXPECT_EQ(expect_free_count, ram_page_free_fake.call_count);
    EXPECT_EQ(1, arr_free_fake.call_count);
    ASSERT_TEMP_MAP_BALANCED();
}

// Process Set Entrypoint

TEST_F(Process, process_set_entrypoint_InvalidParameters) {
    EXPECT_NE(0, process_set_entrypoint(0, 0));
    EXPECT_NE(0, process_set_entrypoint(&proc, 0));
    EXPECT_NE(0, process_set_entrypoint(0, (void *)1));
    ASSERT_TEMP_MAP_BALANCED();
}

TEST_F(Process, process_set_entrypoint_AlreadyRunning) {
    proc.state = PROCESS_STATE_SUSPENDED;
    EXPECT_NE(0, process_set_entrypoint(&proc, 0));
    EXPECT_NE(0, process_set_entrypoint(0, (void *)1));
    EXPECT_NE(0, process_set_entrypoint(&proc, (void *)1));
    ASSERT_TEMP_MAP_BALANCED();
}

TEST_F(Process, process_set_entrypoint_FailMapDir) {
    paging_temp_map_fake.return_val = 0;
    EXPECT_NE(0, process_set_entrypoint(&proc, (void *)1));
    EXPECT_EQ(1, paging_temp_map_fake.call_count);
    ASSERT_TEMP_MAP_BALANCE_OFFSET(1);
}

TEST_F(Process, process_set_entrypoint_FailMapTable) {
    void * paging_temp_map_seq[2] = {(void *)2, 0};
    SET_RETURN_SEQ(paging_temp_map, paging_temp_map_seq, 2);

    EXPECT_NE(0, process_set_entrypoint(&proc, (void *)1));
    EXPECT_EQ(2, paging_temp_map_fake.call_count);
    ASSERT_TEMP_MAP_BALANCE_OFFSET(1);
}

TEST_F(Process, process_set_entrypoint_FailMapStack) {
    void * paging_temp_map_seq[3] = {(void *)2, (void *)3, 0};
    SET_RETURN_SEQ(paging_temp_map, paging_temp_map_seq, 3);

    EXPECT_NE(0, process_set_entrypoint(&proc, (void *)1));
    EXPECT_EQ(3, paging_temp_map_fake.call_count);
    ASSERT_TEMP_MAP_BALANCE_OFFSET(1);
}

TEST_F(Process, process_set_entrypoint_FailFrame) {
    // argc/argv write succeeds (3 temp maps), ring 3 launch frame write fails
    // on its last (page) temp map
    void * paging_temp_map_seq[6] = {
        heap_data.data(),
        heap_data.data(),
        heap_data.data(),
        &dir,
        &table,
        0,
    };
    SET_RETURN_SEQ(paging_temp_map, paging_temp_map_seq, 6);

    EXPECT_NE(0, process_set_entrypoint(&proc, (void *)1));
    EXPECT_EQ(6, paging_temp_map_fake.call_count);
    ASSERT_TEMP_MAP_BALANCE_OFFSET(1);
}

TEST_F(Process, process_set_entrypoint) {
    proc.argc = 3;
    proc.argv = (char **)0x500000;
    proc.esp0 = (uint32_t)temp_page.data() + temp_page.size() - 1;

    void * paging_temp_map_seq[6] = {
        heap_data.data(),
        heap_data.data(),
        heap_data.data(), // argc/argv write
        temp_page.data(),
        temp_page.data(),
        temp_page.data(), // ring 3 launch frame write
    };
    SET_RETURN_SEQ(paging_temp_map, paging_temp_map_seq, 6);

    EXPECT_EQ(0, process_set_entrypoint(&proc, (void *)0x400000));

    uint32_t   user_esp   = VADDR_USER_STACK - 8 + 1;
    uint32_t * args_stack = (uint32_t *)heap_data.data();
    uint32_t   args_first = (user_esp % PAGE_SIZE) / 4;
    EXPECT_EQ(3u, args_stack[args_first + 0]);        // argc
    EXPECT_EQ(0x500000u, args_stack[args_first + 1]); // argv

    // 10 dwords (40 bytes), esp0 is "last byte inclusive" so subtract 39
    uint32_t frame_base = proc.esp0 - 39;
    EXPECT_EQ(frame_base, proc.esp);

    uint32_t * stack   = (uint32_t *)temp_page.data();
    uint32_t   first_i = (frame_base % PAGE_SIZE) / 4;

    EXPECT_EQ(0u, stack[first_i + 0]); // dummy eax (popped by switch_task.resume)
    EXPECT_EQ(0u, stack[first_i + 1]); // dummy esi
    EXPECT_EQ(0u, stack[first_i + 2]); // dummy edi
    EXPECT_EQ(0u, stack[first_i + 3]); // dummy ebp
    EXPECT_EQ(PTR2UINT(enter_usermode), stack[first_i + 4]);
    EXPECT_EQ(0x400000u, stack[first_i + 5]);                        // iret: eip
    EXPECT_EQ((uint32_t)GDT_SELECTOR_USER_CODE, stack[first_i + 6]); // iret: cs
    EXPECT_EQ(0x202u, stack[first_i + 7]);                           // iret: eflags
    EXPECT_EQ(user_esp, stack[first_i + 8]);                         // iret: esp
    EXPECT_EQ((uint32_t)GDT_SELECTOR_USER_DATA, stack[first_i + 9]); // iret: ss
}

// Process Resume

TEST_F(Process, process_resume_InvalidParameters) {
    EXPECT_NE(0, process_resume(0, 0));
    EXPECT_EQ(0, tss_set_esp0_fake.call_count);
    EXPECT_EQ(0, mmu_change_dir_fake.call_count);
    proc.state = PROCESS_STATE_DEAD;
    EXPECT_NE(0, process_resume(0, 0));
}

TEST_F(Process, process_resume_ProcessDead) {
    proc.state = PROCESS_STATE_DEAD;
    EXPECT_NE(0, process_resume(&proc, 0));
    proc.state = PROCESS_STATE_ERROR;
    EXPECT_NE(0, process_resume(&proc, 0));
}

TEST_F(Process, process_resume_ProcessUnloaded) {
    proc.state = PROCESS_STATE_LOADING;
    EXPECT_NE(0, process_resume(&proc, 0));
}

static void customswitch_task(process_t *) {
    ASSERT_EQ(PROCESS_STATE_RUNNING, proc.state);
    ASSERT_EQ(PROCESS_STATE_SUSPENDED, alt_proc.state);
}

TEST_F(Process, process_resume) {
    proc.esp                        = 1;
    proc.esp0                       = 3;
    proc.cr3                        = 4;
    switch_task_fake.custom_fake    = customswitch_task;
    proc.state                      = PROCESS_STATE_SUSPENDED;
    alt_proc.state                  = PROCESS_STATE_RUNNING;
    get_active_task_fake.return_val = &alt_proc;
    EXPECT_EQ(0, process_resume(&proc, (ebus_event_t *)5));
    ASSERT_EQ(1, switch_task_fake.call_count);
    EXPECT_EQ(&proc, switch_task_fake.arg0_val);
    EXPECT_EQ(PROCESS_STATE_RUNNING, alt_proc.state);
}

// Process Add Pages

TEST_F(Process, process_add_pages_InvalidParameters) {
    EXPECT_EQ(0, process_add_pages(0, 0));
    EXPECT_EQ(0, process_add_pages(0, 1));
    EXPECT_EQ(0, process_add_pages(&proc, 0));

    proc.next_heap_page = MMU_DIR_SIZE * MMU_TABLE_SIZE - 1;

    // Count will pass end of last table
    EXPECT_EQ(0, process_add_pages(&proc, 1));
}

TEST_F(Process, process_add_pages_FailTempMap) {
    paging_temp_map_fake.return_val = 0;

    EXPECT_EQ(0, process_add_pages(&proc, 1));
    ASSERT_TEMP_MAP_BALANCE_OFFSET(1);
}

TEST_F(Process, process_add_pages_FailAddPages) {
    paging_temp_map_fake.return_val  = &dir;
    paging_add_pages_fake.return_val = -1;

    EXPECT_EQ(0, process_add_pages(&proc, 1));
    ASSERT_TEMP_MAP_BALANCED();
}

TEST_F(Process, process_add_pages) {
    paging_temp_map_fake.return_val = &dir;

    int next_heap = proc.next_heap_page;

    EXPECT_NE(nullptr, process_add_pages(&proc, 1));
    EXPECT_EQ(1, paging_add_pages_fake.call_count);
    EXPECT_EQ(next_heap, paging_add_pages_fake.arg1_val);
    EXPECT_EQ(next_heap + 1, paging_add_pages_fake.arg2_val);
    EXPECT_EQ((uint32_t)MMU_TABLE_RW_USER, paging_add_pages_fake.arg3_val);
    EXPECT_EQ(next_heap + 1, proc.next_heap_page);
    ASSERT_TEMP_MAP_BALANCED();
}

// Process Grow Stack

TEST_F(Process, process_grow_stack_InvalidParameters) {
    EXPECT_NE(0, process_grow_stack(0));
}

TEST_F(Process, process_grow_stack_FailTempMap) {
    paging_temp_map_fake.return_val = 0;

    EXPECT_NE(0, process_grow_stack(&proc));
    ASSERT_TEMP_MAP_BALANCE_OFFSET(1);
}

TEST_F(Process, process_grow_stack_FailAddPages) {
    paging_temp_map_fake.return_val  = &dir;
    paging_add_pages_fake.return_val = -1;

    EXPECT_NE(0, process_grow_stack(&proc));
    EXPECT_EQ(1, paging_temp_free_fake.call_count);
    ASSERT_TEMP_MAP_BALANCED();
}

TEST_F(Process, process_grow_stack) {
    paging_temp_map_fake.return_val = &dir;

    EXPECT_EQ(0, process_grow_stack(&proc));
    EXPECT_EQ(1, paging_temp_free_fake.call_count);
    EXPECT_EQ((uint32_t)MMU_TABLE_RW_USER, paging_add_pages_fake.arg3_val);
    // Must count down from the user/ISR stack boundary, not from the
    // absolute top of the address space - otherwise this collides with (and
    // silently converts to user-accessible) the supervisor-only ISR stack.
    EXPECT_EQ((uint32_t)ADDR2PAGE(VADDR_USER_STACK), paging_add_pages_fake.arg1_val);
    EXPECT_EQ((uint32_t)ADDR2PAGE(VADDR_USER_STACK), paging_add_pages_fake.arg2_val);
    ASSERT_TEMP_MAP_BALANCED();
}

TEST_F(Process, process_grow_stack_DoesNotCollideWithIsrStack) {
    // stack_page_count == 1 matches the real state right after
    // process_create (which allocates exactly 1 initial user stack page at
    // ADDR2PAGE(VADDR_USER_STACK)). The next grown page must land strictly
    // below that, never inside the ISR stack range
    // [ADDR2PAGE(VADDR_USER_STACK) + 1, ADDR2PAGE(VADDR_ISR_STACK)].
    proc.stack_page_count           = 1;
    paging_temp_map_fake.return_val = &dir;

    EXPECT_EQ(0, process_grow_stack(&proc));
    EXPECT_LT(paging_add_pages_fake.arg1_val, (uint32_t)ADDR2PAGE(VADDR_USER_STACK));
}

// Process Load Heap

TEST_F(Process, process_load_heap_InvalidParameters) {
    EXPECT_NE(0, process_load_heap(0, 0, 0));
    EXPECT_NE(0, process_load_heap(&proc, 0, 0));
    EXPECT_NE(0, process_load_heap(0, heap_data.data(), 0));
    EXPECT_NE(0, process_load_heap(0, 0, 1));
    EXPECT_NE(0, process_load_heap(&proc, heap_data.data(), 0));
    EXPECT_NE(0, process_load_heap(&proc, 0, 1));
    EXPECT_NE(0, process_load_heap(0, heap_data.data(), 1));
}

TEST_F(Process, process_load_heap_FailAddPages) {
    paging_temp_map_fake.return_val = 0;

    EXPECT_NE(0, process_load_heap(&proc, heap_data.data(), PAGE_SIZE));
    EXPECT_EQ(1, paging_temp_map_fake.call_count);
    ASSERT_TEMP_MAP_BALANCE_OFFSET(1);
}

TEST_F(Process, process_load_heap_FailTempMapDir) {
    void * paging_temp_map_seq[2] = {&dir, 0};
    SET_RETURN_SEQ(paging_temp_map, paging_temp_map_seq, 2);

    EXPECT_NE(0, process_load_heap(&proc, heap_data.data(), heap_data.size()));
    EXPECT_EQ(2, paging_temp_map_fake.call_count);
    ASSERT_TEMP_MAP_BALANCE_OFFSET(1);
}

TEST_F(Process, process_load_heap_FailTempMapTable) {
    void * paging_temp_map_seq[3] = {&dir, &dir, 0};
    SET_RETURN_SEQ(paging_temp_map, paging_temp_map_seq, 3);

    EXPECT_NE(0, process_load_heap(&proc, heap_data.data(), heap_data.size()));
    EXPECT_EQ(3, paging_temp_map_fake.call_count);
    ASSERT_TEMP_MAP_BALANCE_OFFSET(1);
}

TEST_F(Process, process_load_heap_FailTempMapPage) {
    void * paging_temp_map_seq[4] = {&dir, &dir, &table, 0};
    SET_RETURN_SEQ(paging_temp_map, paging_temp_map_seq, 4);

    EXPECT_NE(0, process_load_heap(&proc, heap_data.data(), heap_data.size()));
    EXPECT_EQ(4, paging_temp_map_fake.call_count);
    ASSERT_TEMP_MAP_BALANCE_OFFSET(1);
}

TEST_F(Process, process_load_heap_SinglePage) {
    EXPECT_EQ(0, process_load_heap(&proc, heap_data.data(), PAGE_SIZE));
    EXPECT_EQ(1, kmemcpy_fake.call_count);
    EXPECT_EQ(4, paging_temp_map_fake.call_count);
    ASSERT_TEMP_MAP_BALANCED();
}

TEST_F(Process, process_load_heap_MultiplePages) {
    EXPECT_EQ(0, process_load_heap(&proc, heap_data.data(), heap_data.size()));
    EXPECT_EQ(3, kmemcpy_fake.call_count);
    EXPECT_EQ(8, paging_temp_map_fake.call_count);
    ASSERT_TEMP_MAP_BALANCED();
}

TEST_F(Process, process_load_heap_MultipleCalls) {
    EXPECT_EQ(0, process_load_heap(&proc, heap_data.data(), PAGE_SIZE));
    EXPECT_EQ(0, process_load_heap(&proc, heap_data.data(), PAGE_SIZE + 1));

    EXPECT_EQ(3, kmemcpy_fake.call_count);
    EXPECT_EQ(10, paging_temp_map_fake.call_count);
    ASSERT_TEMP_MAP_BALANCED();
}

// Process Copy To Heap

TEST_F(Process, process_copy_to_heap_InvalidParameters) {
    EXPECT_EQ(nullptr, process_copy_to_heap(0, 0, 0));
    EXPECT_EQ(nullptr, process_copy_to_heap(&proc, 0, 0));
    EXPECT_EQ(nullptr, process_copy_to_heap(0, heap_data.data(), 0));
    EXPECT_EQ(nullptr, process_copy_to_heap(0, 0, 1));
    EXPECT_EQ(nullptr, process_copy_to_heap(&proc, heap_data.data(), 0));
    EXPECT_EQ(nullptr, process_copy_to_heap(&proc, 0, 1));
    EXPECT_EQ(nullptr, process_copy_to_heap(0, heap_data.data(), 1));
}

TEST_F(Process, process_copy_to_heap_FailAddPages) {
    paging_temp_map_fake.return_val = 0;

    EXPECT_EQ(nullptr, process_copy_to_heap(&proc, heap_data.data(), PAGE_SIZE));
    ASSERT_TEMP_MAP_BALANCE_OFFSET(1);
}

TEST_F(Process, process_copy_to_heap_FailCopy) {
    void * paging_temp_map_seq[2] = {&dir, 0};
    SET_RETURN_SEQ(paging_temp_map, paging_temp_map_seq, 2);

    EXPECT_EQ(nullptr, process_copy_to_heap(&proc, heap_data.data(), PAGE_SIZE));
    ASSERT_TEMP_MAP_BALANCE_OFFSET(1);
}

TEST_F(Process, process_copy_to_heap) {
    int next_heap = proc.next_heap_page;

    void * result = process_copy_to_heap(&proc, heap_data.data(), PAGE_SIZE);

    EXPECT_EQ(UINT2PTR(PAGE2ADDR(next_heap)), result);
    EXPECT_EQ(1, kmemcpy_fake.call_count);
    ASSERT_TEMP_MAP_BALANCED();
}

// Process Init Memory

TEST_F(Process, process_init_memory_InvalidParameters) {
    EXPECT_NE(0, process_init_memory(0));
}

TEST_F(Process, process_init_memory) {
    EXPECT_EQ(0, process_init_memory(&proc));
    EXPECT_EQ(1, memory_init_fake.call_count);
    EXPECT_EQ(&proc.memory, memory_init_fake.arg0_val);
}

// Process Copy Args

TEST_F(Process, process_copy_args_InvalidParameters) {
    EXPECT_NE(0, process_copy_args(0, 0, 0, 0));
    EXPECT_NE(0, process_copy_args(&proc, 0, 0, 0));
    EXPECT_NE(0, process_copy_args(0, "init", 0, 0));
    EXPECT_NE(0, process_copy_args(&proc, "init", 1, 0));
}

TEST_F(Process, process_copy_args_FailCopyFilepath) {
    kmalloc_fake.return_val = 0;

    EXPECT_NE(0, process_copy_args(&proc, "init", 0, 0));
}

TEST_F(Process, process_copy_args_FailScratchAlloc) {
    kmemcpy_fake.return_val = heap_data.data();

    void * kmalloc_seq[2] = {heap_data.data(), 0};
    SET_RETURN_SEQ(kmalloc, kmalloc_seq, 2);

    EXPECT_NE(0, process_copy_args(&proc, "init", 0, 0));
    EXPECT_EQ(1, kfree_fake.call_count); // frees proc->filepath
}

TEST_F(Process, process_copy_args_FailAddPages) {
    // process_add_pages is a real call here (same file as the function under
    // test), so its failure must be induced via the paging mocks it uses
    kmalloc_fake.return_val         = heap_data.data();
    kmemcpy_fake.return_val         = heap_data.data();
    paging_temp_map_fake.return_val = 0;

    EXPECT_NE(0, process_copy_args(&proc, "init", 0, 0));
    EXPECT_EQ(2, kfree_fake.call_count); // frees scratch buffer + proc->filepath
}

TEST_F(Process, process_copy_args_FailCopyToProcessPages) {
    kmalloc_fake.return_val = heap_data.data();
    kmemcpy_fake.return_val = heap_data.data();

    // First temp map (inside process_add_pages) succeeds, second (the first
    // one inside copy_to_process_pages) fails
    void * paging_temp_map_seq[2] = {&dir, 0};
    SET_RETURN_SEQ(paging_temp_map, paging_temp_map_seq, 2);

    EXPECT_NE(0, process_copy_args(&proc, "init", 0, 0));
    EXPECT_EQ(2, kfree_fake.call_count); // frees scratch buffer + proc->filepath
}

TEST_F(Process, process_copy_args) {
    kmalloc_fake.return_val         = heap_data.data();
    kmemcpy_fake.return_val         = heap_data.data();
    paging_temp_map_fake.return_val = &dir;

    int next_heap = proc.next_heap_page;

    char * argv[2] = {(char *)"foo", (char *)"bar"};

    EXPECT_EQ(0, process_copy_args(&proc, "init", 2, argv));

    // argv[0] (filepath) + 2 provided args
    EXPECT_EQ(3, proc.argc);
    // argv lives in the process' own heap (real process_add_pages, returning
    // the previous next_heap_page as a virtual address), not kernel memory
    EXPECT_EQ((char **)PAGE2ADDR(next_heap), proc.argv);
    EXPECT_EQ(1, kfree_fake.call_count); // only the scratch buffer is freed
}
