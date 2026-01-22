#ifndef FT_ARRAY_H
#define FT_ARRAY_H

#include <stdbool.h>
#include <stddef.h>

typedef enum e_array_type { ARRAY_PATHS, ARRAY_FILES, ARRAY_ARRAY } t_array_type;

typedef struct s_array {
    t_array_type type;
    size_t len;
    size_t cap;
    void **data;
} t_array;

t_array *init_array(size_t size, t_array_type type);
bool append_array(t_array *array, void *content);
void remove_elem_array(t_array *array, const void *content);
void free_array(t_array *array);

#endif // !FT_ARRAY_H
