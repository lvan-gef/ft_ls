#ifndef FT_ARRAY_H
#define FT_ARRAY_H

#include <stdbool.h>
#include <stdint.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE UINT64_C(64)
#endif // ifndef ARRAY_SIZE //

typedef void (*t_array_del)(void *ptr);

typedef struct s_array {
    uint64_t len;
    uint64_t cap;
    void **data;
} t_array;

bool array_init(t_array *array, uint64_t initial_cap);
void array_destroy(t_array *array);
void array_destroy_with(t_array *array, t_array_del del);
bool array_append(t_array *array, void *item);
void *array_pop(t_array *array);
void array_clear(t_array *array);
void array_clear_with(t_array *array, t_array_del del);
void array_reverse(const t_array *array);

#endif // !FT_ARRAY_H
