#include "libc/datastruct/array.mock.h"
#include "libc/datastruct/hash_table.mock.h"

// libc/datastruct/array.h

DEFINE_FAKE_VALUE_FUNC(int, arr_create, arr_t *, size_t, size_t);
DEFINE_FAKE_VOID_FUNC(arr_free, arr_t *);
DEFINE_FAKE_VALUE_FUNC(size_t, arr_size, const arr_t *);
DEFINE_FAKE_VALUE_FUNC(void *, arr_data, const arr_t *);
DEFINE_FAKE_VALUE_FUNC(void *, arr_at, const arr_t *, size_t);
DEFINE_FAKE_VALUE_FUNC(int, arr_set, arr_t *, size_t, const void *);
DEFINE_FAKE_VALUE_FUNC(int, arr_get, const arr_t *, size_t, void *);
DEFINE_FAKE_VALUE_FUNC(int, arr_insert, arr_t *, size_t, const void *);
DEFINE_FAKE_VALUE_FUNC(int, arr_remove, arr_t *, size_t, void *);

void reset_libc_datastruct_array_mock() {
    RESET_FAKE(arr_create);
    RESET_FAKE(arr_free);
    RESET_FAKE(arr_size);
    RESET_FAKE(arr_data);
    RESET_FAKE(arr_at);
    RESET_FAKE(arr_set);
    RESET_FAKE(arr_get);
    RESET_FAKE(arr_insert);
    RESET_FAKE(arr_remove);
}

// libc/datastruct/hash_table.h

DEFINE_FAKE_VALUE_FUNC(int, htable_create, htable_t *, size_t);
DEFINE_FAKE_VOID_FUNC(htable_free, htable_t *);
DEFINE_FAKE_VOID_FUNC(htable_free_no_delete, htable_t *);
DEFINE_FAKE_VALUE_FUNC(int, htable_size, const htable_t *);
DEFINE_FAKE_VALUE_FUNC(int, htable_set, htable_t *, const char *, void *);
DEFINE_FAKE_VALUE_FUNC(void *, htable_get, const htable_t *, const char *);
DEFINE_FAKE_VALUE_FUNC(void *, htable_remove, htable_t *, const char *);
DEFINE_FAKE_VALUE_FUNC(int, htable_delete, htable_t *, const char *);

void reset_libc_datastruct_hash_table_mock() {
    RESET_FAKE(htable_create);
    RESET_FAKE(htable_free);
    RESET_FAKE(htable_free_no_delete);
    RESET_FAKE(htable_size);
    RESET_FAKE(htable_set);
    RESET_FAKE(htable_get);
    RESET_FAKE(htable_remove);
    RESET_FAKE(htable_delete);
}
