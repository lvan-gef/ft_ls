#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <unistd.h>

#include "../include/ft_array.h"
#include "../include/ft_assert.h"

#include "../include/ft_arena.h"
#include "../libft/include/libft.h"

static bool realloc_arr_(t_array *array);

t_array *init_array(Arena *arena, size_t size, t_array_type type) {
    ASSERT_(size, "size must be > 0");
    ASSERT_(type == ARRAY_PATHS || type == ARRAY_FILES,
            "type is not supported");

    t_array *array = ArenaPush(arena, sizeof(*array));
    if (!array) {
        return NULL;
    }

    array->data = (void **)ArenaPush(arena, size * sizeof(*array->data));
    if (!array->data) {
        return NULL;
    }

    array->len = 0;
    array->cap = size;
    array->type = type;
    array->arena = arena;

    return array;
}

bool append_array(t_array *array, void *content) {
    ASSERT_(array, "array can not be NULL");
    ASSERT_(content, "content can not be NULL");

    errno = 0;
    if (array->len == array->cap) {
        if (!realloc_arr_(array)) {
            return false;
        }
    }

    array->data[array->len] = content;
    ++array->len;
    return true;
}

bool insert_array(t_array *array, size_t index, void *content) {
    ASSERT_(array, "array can not be NULL");
    ASSERT_(content, "content can not be NULL");
    ASSERT_(index <= array->len, "index out of bounds");

    errno = 0;
    if (array->len == array->cap) {
        if (!realloc_arr_(array)) {
            return false;
        }
    }

    size_t i = array->len;
    while (i > index) {
        array->data[i] = array->data[i - 1];
        --i;
    }

    array->data[index] = content;
    ++array->len;
    return true;
}

void remove_elem_array(t_array *array, const void *content) {
    ASSERT_(array, "array can not be NULL");
    ASSERT_(content, "content can not be NULL");

    size_t index = 0;
    while (index < array->len) {
        if (array->data[index] == content) {
            size_t next_index = index + 1;
            while (next_index < array->len) {
                array->data[index] = array->data[next_index];
                ++index;
                ++next_index;
            }

            array->data[array->len - 1] = NULL;
            --array->len;
            return;
        }
        ++index;
    }
}

static bool realloc_arr_(t_array *array) {
    ASSERT_(array, "array can not be NULL");

    errno = 0;
    size_t new_cap = array->cap * 2;
    if (new_cap < array->cap) {
        errno = ERANGE;
        return false;
    }

    void **old_data = array->data;
    void **new_data =
        (void **)ArenaPush(array->arena, new_cap * sizeof(*new_data));
    if (!new_data) {
        return false;
    }

    ft_memcpy((void *)new_data, (void *)old_data,
              array->len * sizeof(*array->data));
    array->data = new_data;
    array->cap = new_cap;

    return true;
}
