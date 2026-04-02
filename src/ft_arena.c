#include <stdlib.h>
#include <unistd.h>

#include "../include/ft_arena.h"
#include "../include/ft_assert.h"

#include "../libft/include/libft.h"

static ArenaBlock *new_block_(uint64_t cap);

Arena *ArenaAlloc(uint64_t cap) {
    ASSERT_GT(cap, 0);

    ArenaBlock *block = new_block_(cap);
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

    ASSERT_NOTNULL(arena);
    ASSERT_NOTNULL(arena->first);
    ASSERT_NOTNULL(arena->current);
    ASSERT_EQ(arena->align, 0);
    ASSERT_EQ(arena->block_size, cap);
    return arena;
}

void ArenaRelease(Arena *arena) {
    ASSERT_NOTNULL(arena);

    ArenaBlock *block = arena->first;

    while (block) {
        ArenaBlock *next = block->next;
        free(block);
        block = next;
    }

    free(arena);
}

void ArenaSetAutoAlign(Arena *arena, uint64_t align) {
    ASSERT_NOTNULL(arena);
    ASSERT_GT(align, 0);

    arena->align = align;
    if (align > arena->block_size) {
        arena->block_size = align;
    }
}

ArenaMark ArenaGetMark(const Arena *arena) {
    ASSERT_NOTNULL(arena);

    ArenaMark mark = {.block = arena->current, .pos = arena->current->pos};
    ASSERT_NOTNULL(mark.block);
    ASSERT_LE(mark.pos, mark.block->cap);
    return mark;
}

void *ArenaPushNoZero(Arena *arena, uint64_t size) {
    ASSERT_NOTNULL(arena);
    ASSERT_GT(size, 0);

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

        ArenaBlock *block = new_block_(cap);
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

    ASSERT_NOTNULL(ptr);
    return ptr;
}

void *ArenaPush(Arena *arena, uint64_t size) {
    ASSERT_NOTNULL(arena);
    ASSERT_GT(size, 0);

    void *ptr = ArenaPushNoZero(arena, size);
    if (!ptr) {
        return NULL;
    }

    ft_memset(ptr, 0, size);

    ASSERT_NOTNULL(ptr);
    return ptr;
}

void ArenaPopToMark(Arena *arena, ArenaMark mark) {
    ASSERT_NOTNULL(arena);
    ASSERT_NOTNULL(mark.block);
    ASSERT_LE(mark.pos, mark.block->cap);

    const ArenaBlock *cursor = arena->current;
    while (cursor && cursor != mark.block) {
        cursor = cursor->prev;
    }

    ASSERT_NOTNULL(cursor);
    if (!cursor) {
        return;
    }

    ArenaBlock *block = arena->current;
    while (block != mark.block) {
        ArenaBlock *prev = block->prev;
        if (prev) {
            prev->next = NULL;
        }
        free(block);
        block = prev;
    }

    block->pos = mark.pos;
    arena->current = block;

    ASSERT_NOTNULL(arena->current);
    ASSERT_NULL(arena->current->next);
}

void ArenaClear(Arena *arena) {
    ASSERT_NOTNULL(arena);

    ArenaBlock *block = arena->first->next;

    while (block) {
        ArenaBlock *next = block->next;
        free(block);
        block = next;
    }

    arena->first->next = NULL;
    arena->first->prev = NULL;
    arena->first->pos = 0;
    arena->current = arena->first;
}

static ArenaBlock *new_block_(uint64_t cap) {
    ASSERT_GT(cap, 0);

    ArenaBlock *block = malloc(sizeof(*block) + cap);
    if (!block) {
        return NULL;
    }

    block->next = NULL;
    block->prev = NULL;
    block->pos = 0;
    block->cap = cap;

    ASSERT_NOTNULL(block);
    return block;
}
