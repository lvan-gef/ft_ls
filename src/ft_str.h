#ifndef FT_STR_H
#define FT_STR_H

#include <stddef.h>
#include <stdint.h>

#include "./ft_arena.h"

typedef struct s_str {
    char *str;
    size_t len;
} t_str;

void str_init(t_str *str, char *buf);
void str_copy_cstr(t_str *str, const char *src, size_t len);
void str_copy_uint(t_str *str, size_t value);
size_t str_uint_len(size_t value);
t_str *str_new(size_t cap);
t_str *str_dup(const t_str *src);
void str_free(t_str *str);

t_str *str_arena_new(t_arena *arena, size_t cap);
t_str *str_arena_from_cstr(t_arena *arena, const char *src);
t_str *str_arena_from_uint(t_arena *arena, size_t value);

#endif /* ifndef FT_STR_H */
