#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "./ft_arena.h"

#ifndef ARENA_ALIGN
#define ARENA_ALIGN UINT64_C(8)
#endif /* ifndef ARENA_ALIGN */

static t_arena_block *new_block_(uint64_t cap);

t_arena *arena_alloc(const uint64_t cap) {
    t_arena_block *block = new_block_(cap);
    if (!block) {
        return NULL;
    }

    t_arena *arena = malloc(sizeof(*arena));
    if (!arena) {
        free(block);
        return NULL;
    }

    arena->first = block;
    arena->current = block;
    arena->align = ARENA_ALIGN;
    arena->block_size = cap;

    return arena;
}

void arena_release(t_arena *arena) {
    t_arena_block *block = arena->first;

    while (block) {
        t_arena_block *next = block->next;
        free(block);
        block = next;
    }

    free(arena);
}

t_arena_mark arena_get_mark(const t_arena *arena) {
    const t_arena_mark mark = {.block = arena->current,
                             .pos = arena->current->pos};
    return mark;
}

void *arena_push(t_arena *arena, const uint64_t size) {
    uint64_t align_pos = arena->current->pos;

    const uint64_t remainder = align_pos % arena->align;
    if (remainder) {
        align_pos += arena->align - remainder;
    }

    if (align_pos > arena->current->cap ||
        size > arena->current->cap - align_pos) {
        uint64_t cap = arena->block_size;
        if (size > cap) {
            cap = size;
        }

        t_arena_block *block = new_block_(cap);
        if (!block) {
            return NULL;
        }

        block->prev = arena->current;
        arena->current->next = block;
        arena->current = block;
        align_pos = 0;
    }

    unsigned char *base = (unsigned char *)(arena->current + 1);
    void *ptr = base + align_pos;
    arena->current->pos = align_pos + size;

    return ptr;
}

void arena_pop_to_mark(t_arena *arena, const t_arena_mark mark) {
    const t_arena_block *cursor = arena->current;
    while (cursor && cursor != mark.block) {
        cursor = cursor->prev;
    }

    if (!cursor) {
        return;
    }

    t_arena_block *block = arena->current;
    while (block && block != mark.block) {
        t_arena_block *prev = block->prev;
        if (prev) {
            prev->next = NULL;
        }

        free(block);
        block = prev;
    }

    if (!block) {
        return;
    }

    block->pos = mark.pos;
    arena->current = block;
}

void arena_clear(t_arena *arena) {
    t_arena_block *block = arena->first->next;

    while (block) {
        t_arena_block *next = block->next;
        free(block);
        block = next;
    }

    arena->first->next = NULL;
    arena->first->prev = NULL;
    arena->first->pos = 0;
    arena->current = arena->first;
}

static t_arena_block *new_block_(const uint64_t cap) {
    if (cap > (uint64_t)PTRDIFF_MAX - sizeof(t_arena_block)) {
        errno = ENOMEM;
        return NULL;
    }

    t_arena_block *block = malloc(sizeof(*block) + cap);
    if (!block) {
        return NULL;
    }

    block->next = NULL;
    block->prev = NULL;
    block->pos = 0;
    block->cap = cap;

    return block;
}
