#include <stdlib.h>
#include <unistd.h>

#include "../include/ft_arena.h"
#include "../libft/include/libft.h"
#include "ft_fprintf.h"

static ArenaBlock *new_block_(U64 cap);

Arena *ArenaAlloc(U64 cap) {
    ft_fprintf(STDERR_FILENO, "Alloc new block\n");
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

    return arena;
}

void ArenaRelease(Arena *arena) {
    ArenaBlock *block = arena->first;

    while (block) {
        ArenaBlock *next = block->next;
        free(block);
        block = next;
    }

    free(arena);
}

void ArenaSetAutoAlign(Arena *arena, U64 align) {
    arena->align = align;
    if (align > arena->block_size) {
        arena->block_size = align;
    }
}

U64 ArenaPos(Arena *arena) {
    return arena->current->pos;
}

void *ArenaPushNoZero(Arena *arena, U64 size) {
    U64 align_pos = arena->current->pos;

    if (arena->align) {
        U64 remainder = align_pos % arena->align;
        if (remainder) {
            align_pos += arena->align - remainder;
        }
    }

    if (align_pos + size > arena->current->cap) {
        U64 cap = arena->block_size;
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
    return ptr;
}

void *ArenaPush(Arena *arena, U64 size) {
    void *ptr = ArenaPushNoZero(arena, size);
    if (!ptr) {
        return NULL;
    }

    ft_memset(ptr, 0, size);
    return ptr;
}

void *ArenaPushAligner(Arena *arena, U64 alignment) {
    U64 remainder = arena->current->pos % alignment;

    if (remainder) {
        arena->current->pos += alignment - remainder;
    }

    return ArenaPush(arena, alignment);
}

void ArenaPopTo(Arena *arena, U64 pos) {
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

void ArenaPop(Arena *arena, U64 size) {
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

static ArenaBlock *new_block_(U64 cap) {
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
