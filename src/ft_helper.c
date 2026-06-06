#include <stdint.h>

#include "../include/ft_helper.h"

uint64_t len_of_nbr(uint64_t nbr) {
    uint64_t len = 1;

    while (nbr >= 10) {
        nbr /= 10;
        ++len;
    }

    return len;
}

void free_alloc(const t_alloc *alloc, Arena_Mark mark, void *ptr,
                t_fl_cleanup fl_cleanup) {
    if (!ptr) {
        return;
    }

    switch (alloc->kind) {
        case ALLOC_ARENA: arena_pop_to_mark(alloc->as.arena, mark); break;
        case ALLOC_FL: fl_cleanup(alloc->as.fl, ptr); break;
    }
}
