#include <stdlib.h>
#include <unistd.h>

#include "../include/ft_arena.h"
#include "../include/ft_assert.h"

#include "../libft/include/ft_fprintf.h"
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

uint64_t ArenaPos(Arena *arena) {
    ASSERT_NOTNULL(arena);

    return arena->current->pos;
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

#ifndef NDEBUG
        ft_fprintf(STDERR_FILENO, "No room left\n");
#endif  // NDEBUG

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

void *ArenaPushAligner(Arena *arena, uint64_t alignment) {
    ASSERT_NOTNULL(arena);
    ASSERT_GT(alignment, 0);

    uint64_t remainder = arena->current->pos % alignment;

    if (remainder) {
        arena->current->pos += alignment - remainder;
    }

    return ArenaPush(arena, alignment);
}

void ArenaPopTo(Arena *arena, uint64_t pos) {
    ASSERT_NOTNULL(arena);

    if (pos < arena->current->pos) {
        arena->current->pos = pos;
    }

    if (!arena->current->pos && arena->current->prev) {
        ArenaBlock *block = arena->current;
        arena->current = block->prev;
        arena->current->next = NULL;
        free(block);
    }
}

void ArenaPop(Arena *arena, uint64_t size) {
    ASSERT_NOTNULL(arena);
    ASSERT_GT(size, 0);

    if (size >= arena->current->pos) {
        arena->current->pos = 0;
    } else {
        arena->current->pos -= size;
    }

    if (!arena->current->pos && arena->current->prev) {
        ArenaBlock *block = arena->current;
        arena->current = block->prev;
        arena->current->next = NULL;
        free(block);
    }
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
#ifndef NDEBUG
        ft_fprintf(STDERR_FILENO, "Alloc new block with size: %d\n", cap);
#endif  // NDEBUG
   ASSERT_GT(cap, 0);

    ArenaBlock *block = malloc(sizeof(*block) + cap);
    if (!block) {
        return NULL;
    }

    block->next = NULL;
    block->prev = NULL;
    block->pos = 0;
    block->cap = cap;

    return block;
}
