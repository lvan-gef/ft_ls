#ifndef FT_ARRAY_H
#define FT_ARRAY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "./ft_arena.h"

typedef enum {
    ARRAY_PATHS,
    ARRAY_FILES,
} t_array_type;

typedef struct s_array {
    t_array_type type;
    uint64_t len;
    uint64_t cap;
    void **data;
    Arena *arena;
} t_array;

t_array *init_array(Arena *arena, uint64_t size, t_array_type type);
bool append_array(t_array *array, void *content);
bool insert_array(t_array *array, void *content, uint64_t index);
void remove_elem_array(t_array *array, const void *content);

#endif // !FT_ARRAY_H
