#include "libc/datastruct/array.h"

#include "libc/memory.h"
#include "libc/string.h"

static void * arr_at_no_limit(const arr_t * arr, size_t i);
static int    grow_array(arr_t * arr);

int arr_create(arr_t * arr, size_t size, size_t elem_size) {
    if (!arr || !size || !elem_size) {
        return -1;
    }

    arr->data = pmalloc(size * elem_size);
    if (!arr->data) {
        return -1;
    }

    arr->len       = 0;
    arr->size      = size;
    arr->elem_size = elem_size;

    return 0;
}

void arr_free(arr_t * arr) {
    if (arr && arr->data) {
        pfree(arr->data);
        arr->data = 0;
    }
}

size_t arr_size(const arr_t * arr) {
    return arr->len;
}

void * arr_data(const arr_t * arr) {
    return arr->data;
}

void * arr_at(const arr_t * arr, size_t i) {
    if (!arr || i >= arr->len) {
        return 0;
    }

    return arr_at_no_limit(arr, i);
}

int arr_get(const arr_t * arr, size_t i, void * item) {
    const void * elem = arr_at(arr, i);
    if (!elem) {
        return -1;
    }

    kmemcpy(item, elem, arr->elem_size);

    return 0;
}

int arr_set(arr_t * arr, size_t i, const void * item) {
    void * elem = arr_at(arr, i);
    if (!elem) {
        return -1;
    }

    kmemcpy(elem, item, arr->elem_size);

    return 0;
}

int arr_insert(arr_t * arr, size_t i, const void * item) {
    if (!arr || !item || i > arr->len) {
        return -1;
    }

    if (arr->len >= arr->size) {
        if (grow_array(arr)) {
            return -1;
        }
    }

    void * elem = arr_at_no_limit(arr, i);

    if (i < arr->len) {
        void * shift = elem + arr->elem_size;
        kmemmove(shift, elem, (arr->len - i) * arr->elem_size);
    }

    kmemcpy(elem, item, arr->elem_size);

    arr->len++;

    return 0;
}

int arr_remove(arr_t * arr, size_t i, void * item) {
    void * elem = arr_at(arr, i);
    if (!elem) {
        return -1;
    }

    if (item) {
        kmemcpy(item, elem, arr->elem_size);
    }

    if (i < arr->len - 1) {
        void * shift = elem + arr->elem_size;
        kmemmove(elem, shift, (arr->len - i) * arr->elem_size);
    }

    arr->len--;

    return 0;
}

static void * arr_at_no_limit(const arr_t * arr, size_t i) {
    return arr->data + (i * arr->elem_size);
}

static int grow_array(arr_t * arr) {
    size_t grow_count = arr->size / 2;
    if (!grow_count) {
        grow_count = 1;
    }
    size_t new_size = arr->size + grow_count;
    void * new_data = prealloc(arr->data, new_size * arr->elem_size);
    if (!new_data) {
        return -1;
    }

    arr->data = new_data;
    arr->size = new_size;

    return 0;
}
