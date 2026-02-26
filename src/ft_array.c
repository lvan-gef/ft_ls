#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <unistd.h>

#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_arena.h"

#include "../libft/include/libft.h"

static bool realloc_arr_(t_array *array);

t_array *init_array(Arena *arena, uint64_t size) {
    ASSERT_NOTNULL(arena);
    ASSERT_GT(size, 0);

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
    array->arena = arena;

    ASSERT_NOTNULL(array);
    return array;
}

bool append_array(t_array *array, void *content) {
    ASSERT_NOTNULL(array);
    ASSERT_NOTNULL(content);

    if (array->len == array->cap) {
        if (!realloc_arr_(array)) {
            return false;
        }
    }

    array->data[array->len] = content;
    ++array->len;
    return true;
}

bool insert_array(t_array *array, void *content, uint64_t index) {
    ASSERT_NOTNULL(array);
    ASSERT_GT(index, 0);
    ASSERT_NOTNULL(content);

    if (array->len == array->cap) {
        if (!realloc_arr_(array)) {
            return false;
        }
    }

    uint64_t i = array->len;
    while (i > index) {
        array->data[i] = array->data[i - 1];
        --i;
    }

    array->data[index] = content;
    ++array->len;
    return true;
}

void remove_elem_array(t_array *array, const void *content) {
    ASSERT_NOTNULL(array);
    ASSERT_NOTNULL(content);

    uint64_t index = 0;
    while (index < array->len) {
        if (array->data[index] == content) {
            uint64_t next_index = index + 1;
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

void *pop_array(t_array *array) {
    ASSERT_NOTNULL(array);

    if (!array->len) {
        return NULL;
    }

    --array->len;
    void *elem = array->data[array->len];
    array->data[array->len] = NULL;
    return elem;
}

void clear_array(t_array *array) {
    ASSERT_NOTNULL(array);
    array->len = 0;
}

static bool realloc_arr_(t_array *array) {
    ASSERT_NOTNULL(array);

    uint64_t new_cap = array->cap * 2;
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
