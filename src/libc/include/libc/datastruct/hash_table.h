#ifndef LIBC_DATASTRUCT_HASHTABLE_H
#define LIBC_DATASTRUCT_HASHTABLE_H

#include <stddef.h>

/// Container for each element stored in the hash table
typedef struct _ds_htable_list {
    struct _ds_htable_list * next;
    // htable_t owns both of these pointers
    char * key;
    void * data; // Use htable_remove to get ownership of data so it doesn't get freed
} htable_list_t;

/// The hash table itself
typedef struct _ds_htable {
    htable_list_t ** list;
    size_t           hash_size; // size of list
} htable_t;

/**
 * @brief Create a new hash_table.
 *
 * This table is assumed to own the data pointer for all elements in the table.
 * All data pointers will be freed when the table is freed or when an element
 * is deleted. To remove an element without deleting the data pointer, use
 * `htable_remove`
 *
 * `table` is expected to be a pre-allocated struct
 *
 * Example with static table
 *
 * ```c
 * htable_t ht;
 * htable_create(&ht, 100);
 * ```
 *
 * Example with malloc table
 *
 * ```c
 * htable_t * ht = malloc(sizeof(htable_t));
 * htable_create(ht, 100);
 * ```
 *
 * @param table pointer to the hash table
 * @param hash_size max number of hash values in the table
 * @return int 0 for success
 */
int htable_create(htable_t * table, size_t hash_size);

/**
 * @brief Free the hash table.
 *
 * This will delete the data pointer for all elements in the hash table.
 *
 * @param table pointer to the hash table
 */
void htable_free(htable_t * table);

/**
 * @brief Free the hash table and it's elements.
 *
 * This will NOT delete the data pointer for elements in the hash table.
 *
 * @param table pointer to the hash table
 */
void htable_free_no_delete(htable_t * table);

/**
 * @brief Get the number of elements in the table
 *
 * @param table pointer to the hash table
 * @return int number of elements or < 0 for error
 */
int htable_size(const htable_t * table);

/**
 * @brief Assign a data pointer to key.
 *
 * If a value already exists for `key`, it's data pointer will be freed before
 * assigning to the new `data` value. To avoid this free, the caller should take
 * ownership of the existing data useing htable_remove.
 *
 * IMPORTANT the hash table takes ownership of `data` and will free it when
 * htable_free is called. Use htable_remove to recover ownership of `data`
 * before calling htable_free to avoid it being freed.
 *
 * @param table pointer to the hash table
 * @param key string key
 * @param data pointer to some value
 * @return int 0 for success
 */
int htable_set(htable_t * table, const char * key, void * data);

void * htable_get(const htable_t * table, const char * key);

/**
 * @brief Remove and return an element from the table.
 *
 * This returns the data pointer without freeing it. The caller is now the
 * owner of that pointer and is responsible for freeing it.
 *
 * @param table pointer to the hash table
 * @param key string key
 * @return void* pointer to data, 0 for error or does not exist
 */
void * htable_remove(htable_t * table, const char * key);

/**
 * @brief Remove an element and free it's data pointer.
 *
 * This will call free on the data pointer.
 *
 * IMPORTANT the hash table has ownership of `data` and will free it when this
 * is called. Use htable_remove instead to recover ownership of `data`.
 *
 * @param table pointer to the hash table
 * @param key string key
 * @return int 0 for success
 */
int htable_delete(htable_t * table, const char * key);

#endif // LIBC_DATASTRUCT_HASHTABLE_H
