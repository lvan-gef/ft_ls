#include <stdlib.h>
#include <unistd.h>

#include "../include/ft_arena.h"

static Arena_Block *new_block_(uint64_t cap);

Arena *arena_alloc(uint64_t cap) {
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
    arena->align = 0;
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

void arena_auto_align(Arena *arena, uint64_t align) {
    arena->align = align;
    if (align > arena->block_size) {
        arena->block_size = align;
    }
}

Arena_Mark arena_get_mark(const Arena *arena) {
    Arena_Mark mark = {.block = arena->current, .pos = arena->current->pos};
    return mark;
}

void *arena_push_no_zero(Arena *arena, uint64_t size) {
    uint64_t align_pos = arena->current->pos;

    if (arena->align) {
        uint64_t remainder = align_pos % arena->align;
        if (remainder) {
            align_pos += arena->align - remainder;
        }
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

void arena_pop_to_mark(Arena *arena, Arena_Mark mark) {
    const Arena_Block *cursor = arena->current;
    while (cursor && cursor != mark.block) {
        cursor = cursor->prev;
    }

    if (!cursor) {
        return;
    }

    Arena_Block *block = arena->current;
    while (block != mark.block) {
        Arena_Block *prev = block->prev;
        if (prev) {
            prev->next = NULL;
        }
        free(block);
        block = prev;
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

static Arena_Block *new_block_(uint64_t cap) {
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
