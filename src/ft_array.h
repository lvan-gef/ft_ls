#ifndef FT_ARRAY_H
#define FT_ARRAY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE 64
#endif /* ifndef ARRAY_SIZE */

#ifndef MAX_ALLOC_SIZE
#define MAX_ALLOC_SIZE ((PTRDIFF_MAX / sizeof(void *)))
#endif /* ifndef MAX_ALLOC_SIZE */

typedef void (*t_array_del)(void *ptr);

typedef struct s_array {
    size_t len;
    size_t cap;
    void **data;
} t_array;

bool array_init(t_array *array, size_t initial_cap);
void array_reverse(const t_array *array);
void array_destroy(t_array *array);
void array_destroy_with(t_array *array, t_array_del del);
bool array_append(t_array *array, void *item);
void *array_pop(t_array *array);
void array_clear(t_array *array);
void array_clear_with(t_array *array, t_array_del del);

#endif /* ifndef FT_ARRAY_H */
