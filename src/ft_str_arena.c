#include <stddef.h>
#include <stdint.h>

#include "./ft_arena.h"
#include "./ft_str.h"
#include "./ft_str_arena.h"
#include "./ft_utils.h"

t_str *str_arena_new(t_arena *arena, const size_t cap) {
    t_str *str = arena_push(arena, sizeof(*str) + cap + 1);
    if (!str) {
        return NULL;
    }

    str_init(str, (char *)(str + 1), cap);
    return str;
}

t_str *str_arena_from_cstr(t_arena *arena, const char *src) {
    const size_t len = ft_strlen(src);
    t_str *str = str_arena_new(arena, len);
    if (!str) {
        return NULL;
    }

    str_copy_cstr(str, src, len);
    return str;
}

t_str *str_arena_from_uint(t_arena *arena, const size_t value) {
    const size_t len = str_uint_len(value);
    t_str *str = str_arena_new(arena, len);
    if (!str) {
        return NULL;
    }

    str_copy_uint(str, value);
    return str;
}
