#ifndef ARENA_H
#define ARENA_H

#include <stdint.h>

typedef struct Arena_block Arena_Block;
struct Arena_block {
    uint64_t pos;
    uint64_t cap;
    Arena_Block *next;
    Arena_Block *prev;
};

typedef struct {
    uint64_t align;
    uint64_t block_size;
    Arena_Block *first;
    Arena_Block *current;
} Arena;

typedef struct {
    Arena_Block *block;
    uint64_t pos;
} Arena_Mark;

#ifndef ARENA_SIZE
#define ARENA_SIZE UINT64_C(4096)
#endif // !ARENA_SIZE

Arena *arena_alloc(uint64_t cap);
void arena_release(Arena *arena);
void arena_auto_align(Arena *arena, uint64_t align);
Arena_Mark arena_get_mark(const Arena *arena);
void *arena_push(Arena *arena, uint64_t size);
void arena_pop_to_mark(Arena *arena, Arena_Mark mark);
void arena_clear(Arena *arena);

#endif // !ARENA_H
