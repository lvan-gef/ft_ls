#include <stdint.h>

#include "../include/ft_arena.h"
#include "../include/ft_free_list.h"
#include "../include/ft_helper.h"

uint64_t len_of_nbr(uint64_t nbr) {
    uint64_t len = 1;

    while (nbr >= 10) {
        nbr /= 10;
        ++len;
    }

    return len;
}

void *alloc_mem(const t_alloc *alloc, Arena_Mark *mark, const uint64_t size) {
    switch (alloc->kind) {
        case ALLOC_ARENA:
            if (mark) {
                *mark = arena_get_mark(alloc->as.arena);
            }
            return arena_push(alloc->as.arena, size);
        case ALLOC_FL: return fl_alloc(alloc->as.fl, size);
        default: return NULL;
    }
}

void free_alloc(const t_alloc *alloc, const Arena_Mark mark, void *ptr,
                const t_fl_cleanup fl_cleanup) {
    if (!ptr) {
        return;
    }

    switch (alloc->kind) {
        case ALLOC_ARENA: arena_pop_to_mark(alloc->as.arena, mark); break;
        case ALLOC_FL: fl_cleanup(alloc->as.fl, ptr); break;
    }
}
