#ifndef FT_ARRAY_H
#define FT_ARRAY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE 64
#endif /* ifndef ARRAY_SIZE */

typedef struct s_array {
    size_t len;
    size_t cap;
    void **data;
} t_array;

bool array_init(t_array *array, size_t initial_cap);
void array_reverse(const t_array *array);
void array_destroy(t_array *array);
bool array_append(t_array *array, void *item);
void *array_pop(t_array *array);
void array_clear(t_array *array);

#endif /* ifndef FT_ARRAY_H */
