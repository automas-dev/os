
#include "paging.mock.h"

DEFINE_FAKE_VALUE_FUNC(int, paging_init);
DEFINE_FAKE_VALUE_FUNC(void *, paging_temp_map, uint32_t);
DEFINE_FAKE_VALUE_FUNC(int, paging_temp_free, uint32_t);
DEFINE_FAKE_VALUE_FUNC(size_t, paging_temp_available);
DEFINE_FAKE_VALUE_FUNC(int, paging_id_map_range, size_t, size_t);
DEFINE_FAKE_VALUE_FUNC(int, paging_id_map_page, size_t);
DEFINE_FAKE_VALUE_FUNC(int, paging_add_pages, mmu_dir_t *, size_t, size_t, uint32_t);
DEFINE_FAKE_VALUE_FUNC(int, paging_remove_pages, mmu_dir_t *, size_t, size_t);
DEFINE_FAKE_VALUE_FUNC(int, paging_add_table, mmu_dir_t *, size_t, uint32_t);
DEFINE_FAKE_VALUE_FUNC(int, paging_remove_table, mmu_dir_t *, size_t);

void reset_paging_mock() {
    RESET_FAKE(paging_init);
    RESET_FAKE(paging_temp_map);
    RESET_FAKE(paging_temp_free);
    RESET_FAKE(paging_temp_available);
    RESET_FAKE(paging_id_map_range);
    RESET_FAKE(paging_id_map_page);
    RESET_FAKE(paging_add_pages);
    RESET_FAKE(paging_remove_pages);
    RESET_FAKE(paging_add_table);
    RESET_FAKE(paging_remove_table);
}
