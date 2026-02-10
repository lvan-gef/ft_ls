#ifndef FT_STR_H
#define FT_STR_H

#include <stddef.h>
#include <stdint.h>

#include "./ft_arena.h"

typedef struct {
    char *str;
    uint64_t cap;
    uint64_t len;
    uint64_t pos;
} t_str;

t_str *init_str(Arena *arena, uint64_t cap);
t_str *create_str(Arena *arena, const char *str);
t_str *dup_str(Arena *arena, const t_str *str);
uint64_t cat_str(t_str *dst, const t_str *src);
uint64_t cat_l_str(t_str *dst, const t_str *src, uint64_t size);
uint64_t cpy_str(t_str *dst, const t_str *src);
uint64_t cpy_l_str(t_str *dst, const t_str *src, uint64_t size);

#endif // !FT_STR_H
