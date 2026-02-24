#ifndef FT_ARRAY_H
#define FT_ARRAY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "./ft_arena.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE UINT64_C(10)
#endif // ifndef ARRAY_SIZE //

typedef struct s_array {
    uint64_t len;
    uint64_t cap;
    void **data;
    Arena *arena;
} t_array;

t_array *init_array(Arena *arena, uint64_t size);
bool append_array(t_array *array, void *content);
bool insert_array(t_array *array, void *content, uint64_t index);
void remove_elem_array(t_array *array, const void *content);
void *pop_array(t_array *array);
void clear_array(t_array *array);

#endif // !FT_ARRAY_H
