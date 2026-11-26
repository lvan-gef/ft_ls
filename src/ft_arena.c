#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "./ft_arena.h"

static t_arena_block *new_block_(size_t cap);

bool arena_init(t_arena *arena, const size_t cap) {
    arena->first = new_block_(cap);
    arena->current = arena->first;
    arena->block_size = cap;
    return arena->first != NULL;
}

void arena_release(t_arena *arena) {
    t_arena_block *block = arena->first;

    while (block) {
        t_arena_block *next = block->next;
        free(block);
        block = next;
    }
}

void *arena_push(t_arena *arena, const size_t size) {
    size_t align_pos = arena->current->pos;

    const size_t remainder = align_pos % ARENA_ALIGN;
    if (remainder) {
        align_pos += ARENA_ALIGN - remainder;
    }

    if (align_pos > arena->current->cap ||
        size > arena->current->cap - align_pos) {
        size_t cap = arena->block_size;
        if (size > cap) {
            cap = size;
        }

        t_arena_block *block = new_block_(cap);
        if (!block) {
            return NULL;
        }

        arena->current->next = block;
        arena->current = block;
        align_pos = 0;
    }

    unsigned char *base = (unsigned char *)(arena->current + 1);
    void *ptr = base + align_pos;
    arena->current->pos = align_pos + size;

    return ptr;
}

void arena_clear(t_arena *arena) {
    if (!arena->first) {
        return;
    }

    t_arena_block *block = arena->first->next;
    while (block) {
        t_arena_block *next = block->next;
        free(block);
        block = next;
    }

    arena->first->next = NULL;
    arena->first->pos = 0;
    arena->current = arena->first;
}

static t_arena_block *new_block_(const size_t cap) {
    t_arena_block *block = malloc(sizeof(*block) + cap);
    if (!block) {
        return NULL;
    }

    block->next = NULL;
    block->pos = 0;
    block->cap = cap;

    return block;
}
