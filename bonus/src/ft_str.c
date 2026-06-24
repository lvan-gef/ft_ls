#include <stdint.h>
#include <stdlib.h>

#include "../include/ft_str.h"

#include "../../libft/include/libft.h"

void str_init(t_str *str, char *buf, const uint64_t cap) {
    str->str = buf;
    str->cap = cap + 1;
    str->len = 0;
    str->str[0] = '\0';
}

void str_copy_cstr(t_str *str, const char *src, const uint64_t len) {
    ft_memcpy(str->str, src, (size_t)len);
    str->len = len;
    str->str[str->len] = '\0';
}

void str_copy_uint(t_str *str, uint64_t value) {
    const uint64_t len = str_uint_len(value);

    str->len = len;
    str->str[len] = '\0';
    for (uint64_t pos = len; pos > 0;) {
        --pos;
        str->str[pos] = (char)((value % 10) + '0');
        value /= 10;
    }
}

t_str *str_new(const uint64_t cap) {
    if (cap > UINT64_MAX - 1 - sizeof(t_str)) {
        return NULL;
    }

    t_str *str = malloc(sizeof(*str) + cap + 1);
    if (!str) {
        return NULL;
    }

    str_init(str, (char *)(str + 1), cap);
    return str;
}

t_str *str_from_cstr(const char *src) {
    const size_t len = ft_strlen(src);

    t_str *new_str = str_new(len);
    if (!new_str) {
        return NULL;
    }

    str_copy_cstr(new_str, src, (uint64_t)len);
    return new_str;
}

t_str *str_dup(const t_str *src) {
    t_str *new_str = str_new(src->cap - 1);
    if (!new_str) {
        return NULL;
    }

    str_copy_cstr(new_str, src->str, src->len);

    return new_str;
}

void str_free(t_str *str) {
    free(str);
}

uint64_t str_uint_len(uint64_t value) {
    uint64_t len = 1;

    while (value >= 10) {
        value /= 10;
        ++len;
    }

    return len;
}
