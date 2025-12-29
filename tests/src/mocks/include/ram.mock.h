#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "drivers/ram.h"
#include "fff.h"

DECLARE_FAKE_VALUE_FUNC(int, ram_init, ram_table_t *, void *);
DECLARE_FAKE_VALUE_FUNC(int, ram_region_add_memory, uint64_t, uint64_t);
DECLARE_FAKE_VALUE_FUNC(size_t, ram_free_pages);
DECLARE_FAKE_VALUE_FUNC(size_t, ram_max_pages);
DECLARE_FAKE_VALUE_FUNC(uint32_t, ram_page_alloc);
DECLARE_FAKE_VALUE_FUNC(uint32_t, ram_page_palloc);
DECLARE_FAKE_VALUE_FUNC(int, ram_page_free, uint32_t);

void reset_ram_mock(void);

#ifdef __cplusplus
} // extern "C"
#endif
