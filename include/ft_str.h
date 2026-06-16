#ifndef FT_STR_H
#define FT_STR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "./ft_free_list.h"
#include "./ft_arena.h"

typedef struct {
    char *str;
    uint64_t cap;
    uint64_t len;
    uint64_t pos;
} t_str;

t_str *arena_init_str(Arena *arena, uint64_t cap);
t_str *fl_init_str(free_list *fl, uint64_t cap);
t_str *arena_create_str(Arena *arena, const char *str);
t_str *fl_create_str(free_list *fl, const char *str);
t_str *dup_str(free_list *fl, const t_str *str);
t_str *uint_to_str(Arena *arena, uint64_t nbr);
void free_str(free_list *fl, const t_str *str);
#endif // !FT_STR_H
