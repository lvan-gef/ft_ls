#ifndef ARENA_H
#define ARENA_H

#include <stdint.h>

typedef struct s_arena_block t_arena_block;
struct s_arena_block {
    uint64_t pos;
    uint64_t cap;
    t_arena_block *next;
    t_arena_block *prev;
};

typedef struct s_arena {
    uint64_t align;
    uint64_t block_size;
    t_arena_block *first;
    t_arena_block *current;
} t_arena;

typedef struct {
    t_arena_block *block;
    uint64_t pos;
} t_arena_mark;

#ifndef ARENA_SIZE
#define ARENA_SIZE UINT64_C(65536)
#endif /* ifndef ARENA_SIZE */

t_arena *arena_alloc(uint64_t cap);
void arena_release(t_arena *arena);
t_arena_mark arena_get_mark(const t_arena *arena);
void *arena_push(t_arena *arena, uint64_t size);
void arena_pop_to_mark(t_arena *arena, t_arena_mark mark);
void arena_clear(t_arena *arena);

#endif /* ifndef ARENA_H */
