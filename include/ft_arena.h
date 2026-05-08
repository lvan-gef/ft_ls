#ifndef ARENA_H
#define ARENA_H

#include <stdint.h>

typedef struct Arena_block arena_block;
struct Arena_block {
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
} Arena;

typedef struct {
    arena_block *block;
    uint64_t pos;
} Arena_mark;

#ifndef ARENA_SIZE
#define ARENA_SIZE UINT64_C(4096)
#endif // !ARENA_SIZE

Arena *arena_alloc(uint64_t cap);
void arena_release(Arena *arena);
void arena_auto_align(Arena *arena, uint64_t align);
Arena_mark arena_get_mark(const Arena *arena);
void *arena_push_no_zero(Arena *arena, uint64_t size);
void *arena_push(Arena *arena, uint64_t size);
void arena_pop_to_mark(Arena *arena, Arena_mark mark);
void arena_clear(Arena *arena);

#endif // !ARENA_H
