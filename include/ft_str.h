#ifndef FT_STR_H
#define FT_STR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "./ft_arena.h"
#include "./ft_free_list.h"

typedef struct {
    char *str;
    uint64_t cap;
    uint64_t len;
    uint64_t pos;
} t_str;

t_str *init_str(free_list *fl, uint64_t cap);
t_str *init_str_arena(Arena *arena, uint64_t cap);
t_str *create_str(free_list *fl, const char *str);
t_str *create_str_arena(Arena *arena, const char *str);
t_str *dup_str(free_list *fl, const t_str *str);
t_str *uint_to_str(free_list *fl, uint64_t nbr);
t_str *uint_to_str_arena(Arena *arena, uint64_t nbr);
void free_str(free_list *fl, t_str *str);
#endif // !FT_STR_H
