#include <errno.h>
#include <stdbool.h>
#include <stddef.h>

#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../libft/include/libft.h"

static bool realloc_arr_(t_array *array);

t_array *init_array(size_t size) {
    CUSTOM_ASSERT_(size, "size must be more then 0");

    t_array *array = ft_calloc(1, sizeof(*array));
    if (!array) {
        return NULL;
    }

    array->data = (void **)malloc(size * sizeof(*array->data));
    if (!array->data) {
        free(array);
        return NULL;
    }
    array->cap = size;

    return array;
}

bool append_array(t_array *array, void *content) {
    errno = 0;
    if (array->len == array->cap) {
        if (!realloc_arr_(array)) {
            return false;
        }
    }

    ((void **)array->data)[array->len] = content;
    array->len++;
    return true;
}

void free_array(t_array *array, void (*free_fn)(void *)) {
    CUSTOM_ASSERT_(array, "array can not be NULL");

    if (free_fn) {
        for (size_t i = 0; i < array->len; i++) {
            free_fn(array->data[i]);
        }
    }
    free((void *)array->data);
    free(array);
}

static bool realloc_arr_(t_array *array) {
    size_t new_cap = array->cap * 2;
    if (new_cap < array->cap) {
        errno = ERANGE;
        return false;
    }

    void **old_data = array->data;
    void **new_data = (void **)malloc(new_cap * sizeof(*new_data));
    if (!new_data) {
        return false;
    }

    ft_memcpy((void *)new_data, (void *)old_data,
              array->len * sizeof(*array->data));
    free((void *)old_data);
    array->data = new_data;
    array->cap = new_cap;

    return true;
}
