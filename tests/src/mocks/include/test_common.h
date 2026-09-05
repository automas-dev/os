#pragma once

#ifdef __cplusplus
#include <gtest/gtest.h>

#define ASSERT_RAM_ALLOC_BALANCED()             ASSERT_EQ(ram_page_alloc_fake.call_count, ram_page_free_fake.call_count)
#define ASSERT_RAM_ALLOC_BALANCE_OFFSET(OFFSET) ASSERT_EQ(ram_page_alloc_fake.call_count, ram_page_free_fake.call_count + (OFFSET))

#define ASSERT_TEMP_MAP_BALANCED()             ASSERT_EQ(paging_temp_map_fake.call_count, paging_temp_free_fake.call_count)
#define ASSERT_TEMP_MAP_BALANCE_OFFSET(OFFSET) ASSERT_EQ(paging_temp_map_fake.call_count, paging_temp_free_fake.call_count + (OFFSET))

extern "C" {
#endif

#include <string.h>

#include "fff.h"
// no sort

// Copy definition from string.h to make vscode happy with fff
extern void * memcpy(void *, const void *, size_t);
extern void * memset(void *, int, size_t);

#include "cpu/mmu.mock.h"
#include "cpu/ports.mock.h"
#include "cpu/tss.mock.h"
#include "ebus.mock.h"
#include "kernel/device/screen.mock.h"
#include "kernel/io/buffer.mock.h"
#include "kernel/memory.mock.h"
#include "kernel/panic.mock.h"
#include "libc/datastruct/array.mock.h"
#include "libc/memory.mock.h"
#include "libc/proc.mock.h"
#include "libc/string.mock.h"
#include "libk/sys_call.mock.h"
#include "log.mock.h"
#include "paging.mock.h"
#include "process.mock.h"
#include "ram.mock.h"

void init_mocks(void);

#ifdef __cplusplus
} // extern "C"
#endif
