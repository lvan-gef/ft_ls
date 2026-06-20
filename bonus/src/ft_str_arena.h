#ifndef FT_STR_ARENA_H
#define FT_STR_ARENA_H

#include <stdint.h>

#include "../include/ft_str.h"

#include "./ft_arena.h"

typedef struct s_arena Arena;
typedef struct s_str t_str;

t_str *str_arena_new(Arena *arena, uint64_t cap);
t_str *str_arena_from_cstr(Arena *arena, const char *src);
t_str *str_arena_from_uint(Arena *arena, uint64_t value);

#endif // !FT_STR_ARENA_H
