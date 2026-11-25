#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "./ft_array.h"
#include "./ft_utils.h"

static bool realloc_arr_(t_array *array);

bool array_init(t_array *array, const size_t initial_cap) {
    array->len = 0;
    array->cap = 0;
    array->data = NULL;

    if (initial_cap == 0) {
        return true;
    }

    array->data = (void **)malloc((size_t)initial_cap * sizeof(*array->data));
    if (!array->data) {
        return false;
    }

    array->cap = initial_cap;
    return true;
}

void array_reverse(const t_array *array) {
    size_t left = 0;
    size_t right = array->len;

    while (left < right) {
        --right;
        void *tmp = array->data[left];
        array->data[left] = array->data[right];
        array->data[right] = tmp;
        ++left;
    }
}

void array_destroy(t_array *array) {
    free((void *)array->data);
    array->data = NULL;
    array->len = 0;
    array->cap = 0;
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
    for (size_t index = 0; index < array->len; ++index) {
        del(array->data[index]);
    }

    array->len = 0;
}

static bool realloc_arr_(t_array *array) {
    size_t new_cap = array->cap ? array->cap * 2 : 1;
    void **old_data = array->data;
    void **new_data = (void **)malloc(new_cap * sizeof(*new_data));
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
