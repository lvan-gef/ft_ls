#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <unistd.h>

#include "../include/ft_arena.h"
#include "../include/ft_array.h"
#include "../include/ft_assert.h"

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
    ASSERT_LE(array->len, array->cap);
    return true;
}

void *pop_array(t_array *array) {
    ASSERT_NOTNULL(array);

    if (!array->len) {
        return NULL;
    }

    ASSERT_LT(array->len - 1, array->len);
    --array->len;
    void *elem = array->data[array->len];
    array->data[array->len] = NULL;
    return elem;
}

t_array *reset_array(Arena *arena) {
    ASSERT_NOTNULL(arena);

    ArenaClear(arena);
    t_array *array = init_array(arena, ARRAY_SIZE);
    if (!array) {
        return NULL;
    }

    ASSERT_EQ(array->len, 0);
    ASSERT_LT(array->len, array->cap);
    return array;
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
