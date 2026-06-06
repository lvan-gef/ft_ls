#ifndef FT_HELPER_H
#define FT_HELPER_H

#include <stdint.h>

#include "./ft_arena.h"
#include "./ft_free_list.h"

typedef enum e_alloc_kind { ALLOC_FL, ALLOC_ARENA } t_alloc_kind;

typedef struct {
    t_alloc_kind kind;
    union {
        free_list *fl;
        Arena *arena;
    } as;
} t_alloc;

typedef void (*t_fl_cleanup)(free_list *fl, void *ptr);

uint64_t len_of_nbr(uint64_t nbr);
void free_alloc(const t_alloc *alloc, Arena_Mark mark, void *ptr,
                t_fl_cleanup fl_cleanup);

#endif // !FT_HELPER_H
