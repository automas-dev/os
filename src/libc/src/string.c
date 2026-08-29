#include "libc/string.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#include "libc/memory.h"

#define BASE_DEFAULT_UPPER true

static char * kstrtok_curr = 0;

int kmemcmp(const void * lhs, const void * rhs, size_t n) {
    if (!lhs || !rhs) {
        return 0;
    }

    const uint8_t * a = (uint8_t *)lhs;
    const uint8_t * b = (uint8_t *)rhs;
    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i]) {
            return (int)a[i] - (int)b[i];
        }
    }
    return 0;
}

void * kmemcpy(void * dest, const void * src, size_t n) {
    if (!dest || !src) {
        return 0;
    }

    uint8_t *       dest_buff = (uint8_t *)dest;
    const uint8_t * src_buff  = (const uint8_t *)src;
    for (size_t i = 0; i < n; i++) {
        *dest_buff++ = *src_buff++;
    }
    return dest;
}

void * kmemmove(void * dest, const void * src, size_t n) {
    if (!dest || !src) {
        return 0;
    }

    uint8_t *       dest_buff = (uint8_t *)dest;
    const uint8_t * src_buff  = (const uint8_t *)src;
    if (dest_buff < src_buff) {
        for (size_t i = 0; i < n; i++) {
            *dest_buff++ = *src_buff++;
        }
    }
    else {
        dest_buff = dest_buff + n - 1;
        src_buff  = src_buff + n - 1;
        for (size_t i = 0; i < n; i++) {
            *dest_buff-- = *src_buff--;
        }
    }
    return dest;
}

void * kmemset(void * ptr, int value, size_t n) {
    if (!ptr) {
        return 0;
    }
    uint8_t * buf = (uint8_t *)ptr;
    for (size_t i = 0; i < n; i++) {
        *buf++ = value;
    }
    return ptr;
}

size_t kstrlen(const char * str) {
    if (!str) {
        return 0;
    }
    size_t count = 0;
    while (*str++) {
        count++;
    }
    return count;
}

size_t knstrlen(const char * str, int max) {
    if (!str || max < 0) {
        return 0;
    }
    size_t count = 0;
    while (*str++ && count < max) {
        count++;
    }
    return count;
}

int kstrcmp(const char * lhs, const char * rhs) {
    size_t lhs_len = kstrlen(lhs);
    size_t rhs_len = kstrlen(rhs);

    if (lhs_len != rhs_len) {
        return lhs_len - rhs_len;
    }

    return kmemcmp(lhs, rhs, lhs_len);
}

char * kstrfind(const char * str, int c) {
    if (!str) {
        return 0;
    }
    size_t len = kstrlen(str);
    for (size_t i = 0; i < len; i++) {
        if (str[i] == (char)c) {
            return (char *)(str + i);
        }
    }
    return 0;
}

// static char * find_one(const char * str, const char * delim) {
//     if (!str || !delim) {
//         return 0;
//     }

//     size_t len   = kstrlen(str);
//     size_t d_len = kstrlen(delim);

//     for (size_t i = 0; i < len; i++) {
//         for (size_t d = 0; d < d_len; d++) {
//             if (str[i] == delim[d]) {
//                 return (char *)(str + i);
//             }
//         }
//     }
//     return (char *)str;
// }

// static char * find_not_one(const char * str, const char * delim) {
//     if (!str || !delim) {
//         return 0;
//     }

//     size_t len   = kstrlen(str);
//     size_t d_len = kstrlen(delim);

//     for (size_t i = 0; i < len; i++) {
//         for (size_t d = 0; d < d_len; d++) {
//             if (str[i] != delim[d]) {
//                 return (char *)(str + i);
//             }
//         }
//     }
//     return (char *)str;
// }

// // TODO Untested, idk if this actually works
// char * kstrtok(char * str, const char * delim) {
//     if (!str || !delim) {
//         return 0;
//     }

//     if (str) {
//         kstrtok_curr = str;
//     }

//     if (kstrtok_curr == 0) {
//         return 0;
//     }

//     kstrtok_curr = find_not_one(kstrtok_curr, delim);
//     size_t len   = kstrlen(kstrtok_curr);
//     if (len == 0) {
//         return 0;
//     }

//     str = kstrtok_curr;

//     kstrtok_curr = find_one(kstrtok_curr, delim);
//     if (kstrtok_curr == str) {
//         kstrtok_curr += len;
//     }

//     *kstrtok_curr = 0;
//     return str;
// }

int katoi(const char * str) {
    if (!str) {
        return 0;
    }

    bool neg = false;
    if (*str == '+') {
        str++;
    }
    else if (*str == '-') {
        neg = true;
        str++;
    }

    int res = 0;
    while (*str) {
        char c = *str++;
        if (c < '0' || c > '9') {
            return 0;
        }

        res *= 10;
        res += (c - '0');
    }

    if (neg) {
        res = -res;
    }

    return res;
}

size_t itoa(int32_t n, char * str) {
    return itoa_base(-1, n, str, 10, BASE_DEFAULT_UPPER);
}

size_t itoa_base(size_t max_length, int32_t n, char * str, uint8_t base, bool upper) {
    if (!str) {
        return 0;
    }

    bool is_neg = n < 0;

    size_t len = 0;

    if (is_neg) {
        *str++ = '-';
        len++;
        n = -n;
    }

    len += utoa_base(max_length - 1, n, str, base, upper);

    return len;
}

size_t ltoa(int64_t n, char * str) {
    return ltoa_base(-1, n, str, 10, BASE_DEFAULT_UPPER);
}

size_t ltoa_base(size_t max_length, int64_t n, char * str, uint8_t base, bool upper) {
    if (!str) {
        return 0;
    }

    bool is_neg = n < 0;

    size_t len = 0;

    if (is_neg) {
        *str++ = '-';
        len++;
        n = -n;
    }

    len += ultoa_base(max_length - 1, n, str, base, upper);

    return len;
}

size_t utoa(uint32_t n, char * str) {
    return utoa_base(-1, n, str, 10, BASE_DEFAULT_UPPER);
}

size_t utoa_base(size_t max_length, uint32_t n, char * str, uint8_t base, bool upper) {
    if (!str || !max_length || !base) {
        return 0;
    }

    size_t   len = 0;
    uint32_t rev = 0;
    while (n > 0) {
        rev = (rev * base) + (n % base);
        n /= base;
        len += 1;
    }

    if (len > max_length) {
        len = max_length;
    }

    for (size_t i = 0; i < len; i++) {
        char digit = rev % base;
        if (digit > 9) {
            digit = (upper ? 'A' : 'a') + (digit - 10);
        }
        else {
            digit = '0' + digit;
        }
        *str++ = digit;
        rev /= base;
    }

    if (len == 0) {
        *str++ = '0';
        len++;
    }

    *str = 0;

    return len;
}

size_t ultoa(uint64_t n, char * str) {
    return ultoa_base(-1, n, str, 10, BASE_DEFAULT_UPPER);
}

size_t ultoa_base(size_t max_length, uint64_t n, char * str, uint8_t base, bool upper) {
    if (!str || !max_length || !base) {
        return 0;
    }

    size_t   len = 0;
    uint64_t rev = 0;
    while (n > 0) {
        rev = (rev * base) + (n % base);
        n /= base;
        len += 1;
    }

    if (len > max_length) {
        len = max_length;
    }

    for (size_t i = 0; i < len; i++) {
        char digit = rev % base;
        if (digit > 9) {
            digit = (upper ? 'A' : 'a') + (digit - 10);
        }
        else {
            digit = '0' + digit;
        }
        *str++ = digit;
        rev /= base;
    }

    if (len == 0) {
        *str++ = '0';
        len++;
    }

    *str = 0;

    return len;
}

char * str_copy(const char * str) {
    if (!str) {
        return 0;
    }

    size_t len = kstrlen(str);
    char * new = pmalloc(len + 1);
    if (new) {
        kmemcpy(new, str, len + 1);
    }
    return new;
}
