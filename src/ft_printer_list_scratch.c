#include <stdint.h>

#include "ft_printer_list_scratch.h"

#include "../libft/include/libft.h"

t_str *printer_scratch_str_new(Arena *scratch, const uint64_t cap) {
    if (cap > UINT64_MAX - (uint64_t)sizeof(t_str) - 1) {
        return NULL;
    }

    t_str *str = arena_push(scratch, sizeof(*str) + cap + 1);
    if (!str) {
        return NULL;
    }

    str_init(str, (char *)(str + 1), cap);
    return str;
}

t_str *printer_scratch_str_from_cstr(Arena *scratch, const char *src) {
    const uint64_t len = (uint64_t)ft_strlen(src);
    t_str *str = printer_scratch_str_new(scratch, len);
    if (!str) {
        return NULL;
    }

    str_copy_cstr(str, src, len);
    return str;
}

t_str *printer_scratch_str_from_uint(Arena *scratch, uint64_t value) {
    const uint64_t len = str_uint_len(value);
    t_str *str = printer_scratch_str_new(scratch, len);
    if (!str) {
        return NULL;
    }

    str_copy_uint(str, value);

    return str;
}
