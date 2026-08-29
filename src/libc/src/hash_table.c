#include "libc/datastruct/hash_table.h"

#include "libc/memory.h"
#include "libc/string.h"

// See https://stackoverflow.com/a/4384446/1947604

static size_t hash(const char * key, size_t hash_size);

int htable_create(htable_t * table, size_t hash_size) {
    if (!table || !hash_size) {
        return -1;
    }

    table->hash_size = hash_size;
    table->list      = pmalloc(sizeof(htable_list_t *) * hash_size);

    if (!table->list) {
        return -1;
    }

    kmemset(table->list, 0, sizeof(htable_list_t *) * hash_size);

    return 0;
}

void htable_free(htable_t * table) {
    if (table && table->list) {
        for (size_t i = 0; i < table->hash_size; i++) {
            htable_list_t * list = table->list[i];
            while (list) {
                htable_list_t * next = list->next;

                if (list->key) {
                    pfree(list->key);
                }
                if (list->data) {
                    pfree(list->data);
                }
                pfree(list);

                list = next;
            }
        }
        pfree(table->list);
        // Don't free table itself. that is owned by the caller and probably static
    }
}

void htable_free_no_delete(htable_t * table) {
    if (table && table->list) {
        for (size_t i = 0; i < table->hash_size; i++) {
            htable_list_t * list = table->list[i];
            while (list) {
                htable_list_t * next = list->next;

                if (list->key) {
                    pfree(list->key);
                }
                // DOES NOT FREE data POINTER
                pfree(list);

                list = next;
            }
        }
        pfree(table->list);
        // Don't free table itself. that is owned by the caller and probably static
    }
}

int htable_size(const htable_t * table) {
    if (!table) {
        return -1;
    }

    int count = 0;
    for (size_t i = 0; i < table->hash_size; i++) {
        const htable_list_t * list = table->list[i];
        while (list) {
            count++;
            list = list->next;
        }
    }

    return count;
}

int htable_set(htable_t * table, const char * key, void * data) {
    if (!table || !key || !data) {
        return -1;
    }

    char * key_copy = str_copy(key);
    if (!key_copy) {
        return -1;
    }

    htable_list_t * list = pmalloc(sizeof(htable_list_t));
    if (!list) {
        pfree(key_copy);
        return -1;
    }

    size_t hash_key = hash(key, table->hash_size);

    list->key  = key_copy;
    list->data = data;
    list->next = table->list[hash_key];

    table->list[hash_key] = list;

    return 0;
}

void * htable_get(const htable_t * table, const char * key) {
    if (!table || !key) {
        return 0;
    }

    size_t hash_key = hash(key, table->hash_size);

    htable_list_t * list = table->list[hash_key];
    while (list) {
        if (kstrcmp(key, list->key) == 0) {
            return list->data;
        }
        list = list->next;
    }

    return 0;
}

void * htable_remove(htable_t * table, const char * key) {
    if (!table || !key) {
        return 0;
    }

    size_t hash_key = hash(key, table->hash_size);

    htable_list_t * list = table->list[hash_key];
    if (!list) {
        return 0;
    }

    if (kstrcmp(key, list->key) == 0) {
        void * data           = list->data;
        table->list[hash_key] = list->next;

        pfree(list->key);
        pfree(list);

        return data;
    }

    htable_list_t * prev = list;
    list                 = list->next;
    while (list) {
        if (kstrcmp(key, list->key) == 0) {
            void * data = list->data;
            prev->next  = list->next;

            pfree(list->key);
            pfree(list);

            return data;
        }
        prev = list;
        list = list->next;
    }

    return 0;
}

int htable_delete(htable_t * table, const char * key) {
    if (!table || !key) {
        return -1;
    }

    size_t hash_key = hash(key, table->hash_size);

    htable_list_t * list = table->list[hash_key];
    if (!list) {
        return -1;
    }

    if (kstrcmp(key, list->key) == 0) {
        void * data           = list->data;
        table->list[hash_key] = list->next;

        pfree(list->data);
        pfree(list->key);
        pfree(list);

        return 0;
    }

    htable_list_t * prev = list;
    list                 = list->next;
    while (list) {
        if (kstrcmp(key, list->key) == 0) {
            void * data = list->data;
            prev->next  = list->next;

            pfree(list->data);
            pfree(list->key);
            pfree(list);

            return 0;
        }
        prev = list;
        list = list->next;
    }

    return -1;
}

static size_t hash(const char * key, size_t hash_size) {
    // Assuming both key and hash_size are valid since this can only be called from functions that already check these
    size_t hashval;
    for (hashval = 0; *key != '\0'; key++) {
        hashval = *key + 31 * hashval;
    }
    return hashval % hash_size;
}
