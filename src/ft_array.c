#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#include "../include/ft_free_list.h"
#include "../include/ft_array.h"
#include "../include/ft_assert.h"

#include "../libft/include/libft.h"

static bool realloc_arr_(t_array *array);

t_array *init_array(free_list *fl, uint64_t size) {
    ASSERT_NOTNULL(fl);
    ASSERT_GT(size, 0);

    t_array *array = free_list_alloc(fl, 1 * sizeof(*array), 8);
    if (!array) {
        return NULL;
    }

    array->data = (void **)free_list_alloc(fl, size * sizeof(*array->data), 8);
    if (!array->data) {
        return NULL;
    }

    array->len = 0;
    array->cap = size;
    array->fl = fl;

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
    ASSERT_GT(array->len, 0);

    --array->len;
    void *elem = array->data[array->len];
    array->data[array->len] = NULL;
    return elem;
}

void reset_array(free_list *fl, t_array *array) {
    ASSERT_NOTNULL(fl);
    ASSERT_NOTNULL(array);

    while (array->len) {
        void *elem = pop_array(array);
        free_list_free(fl, elem);
    }

    ASSERT_EQ(array->len, 0);
    ASSERT_LT(array->len, array->cap);
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
        (void **)free_list_alloc(array->fl, new_cap * sizeof(*new_data), 8);
    if (!new_data) {
        return false;
    }

    ft_memcpy((void *)new_data, (void *)old_data,
              array->len * sizeof(*array->data));
    array->data = new_data;
    array->cap = new_cap;

    return true;
}
