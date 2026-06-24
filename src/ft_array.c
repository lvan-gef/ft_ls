#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "../include/ft_array.h"

#include "../libft/include/libft.h"

#ifndef MAX_ALLOC_SIZE
#define MAX_ALLOC_SIZE ((uint64_t)(PTRDIFF_MAX / sizeof(void *)))
#endif /* ifndef MAX_ALLOC_SIZE */

static bool realloc_arr_(t_array *array);

bool array_init(t_array *array, const uint64_t initial_cap) {
    array->len = 0;
    array->cap = 0;
    array->data = NULL;

    if (initial_cap == 0) {
        return true;
    }

    if (initial_cap > MAX_ALLOC_SIZE) {
        errno = ERANGE;
        return false;
    }

    array->data = (void **)malloc((size_t)initial_cap * sizeof(*array->data));
    if (!array->data) {
        return false;
    }

    array->cap = initial_cap;
    return true;
}

void array_destroy(t_array *array) {
    free((void *)array->data);
    array->data = NULL;
    array->len = 0;
    array->cap = 0;
}

void array_destroy_with(t_array *array, const t_array_del del) {
    for (uint64_t index = 0; index < array->len; ++index) {
        del(array->data[index]);
    }

    array_destroy(array);
}

bool array_append(t_array *array, void *item) {
    if (array->len == array->cap) {
        if (!realloc_arr_(array)) {
            return false;
        }
    }

    array->data[array->len] = item;
    ++array->len;
    return true;
}

void *array_pop(t_array *array) {
    --array->len;
    void *item = array->data[array->len];
    return item;
}

void array_clear(t_array *array) {
    array->len = 0;
}

void array_clear_with(t_array *array, const t_array_del del) {
    while (array->len) {
        --array->len;
        del(array->data[array->len]);
    }
}

static bool realloc_arr_(t_array *array) {
    if (array->cap > MAX_ALLOC_SIZE) {
        errno = ERANGE;
        return false;
    }

    uint64_t new_cap = array->cap ? array->cap * 2 : 1;
    if (new_cap < array->cap || new_cap > MAX_ALLOC_SIZE) {
        new_cap = MAX_ALLOC_SIZE;
    }

    void **old_data = array->data;
    void **new_data = (void **)malloc((size_t)new_cap * sizeof(*new_data));
    if (!new_data) {
        return false;
    }

    if (array->len) {
        ft_memcpy((void *)new_data, (void *)old_data,
                  array->len * sizeof(*array->data));
    }
    free((void *)old_data);
    array->data = new_data;
    array->cap = new_cap;

    return true;
}
