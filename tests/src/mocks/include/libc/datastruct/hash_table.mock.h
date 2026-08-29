#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "fff.h"
#include "libc/datastruct/hash_table.h"

DECLARE_FAKE_VALUE_FUNC(int, htable_create, htable_t *, size_t);
DECLARE_FAKE_VOID_FUNC(htable_free, htable_t *);
DECLARE_FAKE_VOID_FUNC(htable_free_no_delete, htable_t *);
DECLARE_FAKE_VALUE_FUNC(int, htable_size, const htable_t *);
DECLARE_FAKE_VALUE_FUNC(int, htable_set, htable_t *, const char *, void *);
DECLARE_FAKE_VALUE_FUNC(void *, htable_get, const htable_t *, const char *);
DECLARE_FAKE_VALUE_FUNC(void *, htable_remove, htable_t *, const char *);
DECLARE_FAKE_VALUE_FUNC(int, htable_delete, htable_t *, const char *);

void reset_libc_datastruct_hash_table_mock(void);

#ifdef __cplusplus
} // extern "C"
#endif
