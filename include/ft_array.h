#ifndef FT_ARRAY_H
#define FT_ARRAY_H

#include <stdbool.h>
#include <stddef.h>

typedef struct s_array {
    size_t len;
    size_t cap;
    void **data;
} t_array;

t_array *init_array(size_t size);
bool append_array(t_array *array, void *content);
void free_array(t_array *array, void (*free_fn)(void *));

#endif // !FT_ARRAY_H
