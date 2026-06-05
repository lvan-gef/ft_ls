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

typedef enum e_alloc_kind { ALLOC_FL, ALLOC_ARENA } t_alloc_kind;

typedef struct {
    t_alloc_kind kind;
    union {
        free_list *fl;
        Arena *arena;
    } as;
} t_alloc;

t_str *init_str(const t_alloc *alloc, uint64_t cap);
t_str *create_str(const t_alloc *alloc, const char *str);
t_str *dup_str(const t_alloc *alloc, const t_str *str);
t_str *uint_to_str(const t_alloc *alloc, uint64_t nbr);
void free_str(free_list *fl, t_str *str);
#endif // !FT_STR_H
