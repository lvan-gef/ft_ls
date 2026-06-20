#include <stdint.h>
#include <stdlib.h>

#include "./ft_arena.h"

#ifndef ARENA_ALIGN
#define ARENA_ALIGN UINT64_C(8)
#endif // ifndef !ARENA_ALIGN

static Arena_Block *new_block_(uint64_t cap);

Arena *arena_alloc(const uint64_t cap) {
    Arena_Block *block = new_block_(cap);
    if (!block) {
        return NULL;
    }

    Arena *arena = malloc(sizeof(*arena));
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

void arena_release(Arena *arena) {
    Arena_Block *block = arena->first;

    while (block) {
        Arena_Block *next = block->next;
        free(block);
        block = next;
    }

    free(arena);
}

Arena_Mark arena_get_mark(const Arena *arena) {
    const Arena_Mark mark = {.block = arena->current,
                             .pos = arena->current->pos};
    return mark;
}

void *arena_push(Arena *arena, const uint64_t size) {
    uint64_t align_pos = arena->current->pos;

    const uint64_t remainder = align_pos % arena->align;
    if (remainder) {
        align_pos += arena->align - remainder;
    }

    if (align_pos + size > arena->current->cap) {
        uint64_t cap = arena->block_size;
        if (size > cap) {
            cap = size;
        }

        Arena_Block *block = new_block_(cap);
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

void arena_pop_to_mark(Arena *arena, const Arena_Mark mark) {
    const Arena_Block *cursor = arena->current;
    while (cursor && cursor != mark.block) {
        cursor = cursor->prev;
    }

    if (!cursor) {
        return;
    }

    Arena_Block *block = arena->current;
    while (block && block != mark.block) {
        Arena_Block *prev = block->prev;
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

void arena_clear(Arena *arena) {
    Arena_Block *block = arena->first->next;

    while (block) {
        Arena_Block *next = block->next;
        free(block);
        block = next;
    }

    arena->first->next = NULL;
    arena->first->prev = NULL;
    arena->first->pos = 0;
    arena->current = arena->first;
}

static Arena_Block *new_block_(const uint64_t cap) {
    Arena_Block *block = malloc(sizeof(*block) + cap);
    if (!block) {
        return NULL;
    }

    block->next = NULL;
    block->prev = NULL;
    block->pos = 0;
    block->cap = cap;

    return block;
}
