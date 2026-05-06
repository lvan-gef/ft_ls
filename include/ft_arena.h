#ifndef ARENA_H
#define ARENA_H

#include <stdint.h>

typedef struct arena_block arena_block;
struct arena_block {
    uint64_t pos;
    uint64_t cap;
    arena_block *next;
    arena_block *prev;
};

typedef struct {
    uint64_t align;
    uint64_t block_size;
    arena_block *first;
    arena_block *current;
} arena;

typedef struct {
    arena_block *block;
    uint64_t pos;
} arena_mark;

#ifndef ARENA_SIZE
#define ARENA_SIZE UINT64_C(4096)
#endif // !ARENA_SIZE

arena *arena_alloc(uint64_t cap);
void arena_release(arena *arena);
void arena_auto_align(arena *arena, uint64_t align);
arena_mark arena_get_mark(const arena *arena);
void *arena_push_no_zero(arena *arena, uint64_t size);
void *arena_push(arena *arena, uint64_t size);
void arena_pop_to_mark(arena *arena, arena_mark mark);
void arena_clear(arena *arena);

#endif // !ARENA_H
