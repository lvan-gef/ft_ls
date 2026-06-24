#include <stddef.h>
#include <stdint.h>

#include "../include/ft_str.h"

#include "../libft/include/libft.h"

#include "./ft_arena.h"
#include "./ft_str_arena.h"

t_str *str_arena_new(Arena *arena, const uint64_t cap) {
    if (cap > UINT64_MAX - (uint64_t)sizeof(t_str) - 1) {
        return NULL;
    }

    t_str *str = arena_push(arena, sizeof(*str) + cap + 1);
    if (!str) {
        return NULL;
    }

    str_init(str, (char *)(str + 1), cap);
    return str;
}

t_str *str_arena_from_cstr(Arena *arena, const char *src) {
    const uint64_t len = (uint64_t)ft_strlen(src);
    t_str *str = str_arena_new(arena, len);
    if (!str) {
        return NULL;
    }

    str_copy_cstr(str, src, len);
    return str;
}

t_str *str_arena_from_uint(Arena *arena, const uint64_t value) {
    const uint64_t len = str_uint_len(value);
    t_str *str = str_arena_new(arena, len);
    if (!str) {
        return NULL;
    }

    str_copy_uint(str, value);

    return str;
}
