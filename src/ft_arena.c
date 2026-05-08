#include <stdlib.h>
#include <unistd.h>

#include "../include/ft_arena.h"

#include "../libft/include/libft.h"

static arena_block *new_block_(uint64_t cap);

Arena *arena_alloc(uint64_t cap) {
    arena_block *block = new_block_(cap);
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
    arena_block *block = arena->first;

    while (block) {
        arena_block *next = block->next;
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

Arena_mark arena_get_mark(const Arena *arena) {
    Arena_mark mark = {.block = arena->current, .pos = arena->current->pos};
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

        arena_block *block = new_block_(cap);
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

void *arena_push(Arena *arena, uint64_t size) {
    void *ptr = arena_push_no_zero(arena, size);
    if (!ptr) {
        return NULL;
    }

    ft_memset(ptr, 0, size);

    return ptr;
}

void arena_pop_to_mark(Arena *arena, Arena_mark mark) {
    const arena_block *cursor = arena->current;
    while (cursor && cursor != mark.block) {
        cursor = cursor->prev;
    }

    if (!cursor) {
        return;
    }

    arena_block *block = arena->current;
    while (block != mark.block) {
        arena_block *prev = block->prev;
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
    arena_block *block = arena->first->next;

    while (block) {
        arena_block *next = block->next;
        free(block);
        block = next;
    }

    arena->first->next = NULL;
    arena->first->prev = NULL;
    arena->first->pos = 0;
    arena->current = arena->first;
}

static arena_block *new_block_(uint64_t cap) {
    arena_block *block = malloc(sizeof(*block) + cap);
    if (!block) {
        return NULL;
    }

    block->next = NULL;
    block->prev = NULL;
    block->pos = 0;
    block->cap = cap;

    return block;
}
