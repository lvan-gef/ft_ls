#ifndef FT_STR_ARENA_H
#define FT_STR_ARENA_H

#include <stdint.h>

#include "../include/ft_str.h"

#include "./ft_arena.h"

t_str *str_arena_new(t_arena *arena, uint64_t cap);
t_str *str_arena_from_cstr(t_arena *arena, const char *src);
t_str *str_arena_from_uint(t_arena *arena, uint64_t value);

#endif /* ifndef FT_STR_ARENA_H */
