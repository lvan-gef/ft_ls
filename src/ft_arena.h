#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>
#include <stdint.h>

typedef struct s_arena_block t_arena_block;
struct s_arena_block {
    size_t pos;
    size_t cap;
    t_arena_block *next;
    t_arena_block *prev;
};

typedef struct s_arena {
    size_t align;
    size_t block_size;
    t_arena_block *first;
    t_arena_block *current;
} t_arena;

typedef struct {
    t_arena_block *block;
    size_t pos;
} t_arena_mark;

#ifndef ARENA_SIZE
#define ARENA_SIZE 65536
#endif /* ifndef ARENA_SIZE */

#ifndef ARENA_ALIGN
#define ARENA_ALIGN 8
#endif /* ifndef ARENA_ALIGN */

t_arena *arena_alloc(size_t cap);
void arena_release(t_arena *arena);
t_arena_mark arena_get_mark(const t_arena *arena);
void *arena_push(t_arena *arena, size_t size);
void arena_pop_to_mark(t_arena *arena, t_arena_mark mark);
void arena_clear(t_arena *arena);

#endif /* ifndef ARENA_H */
