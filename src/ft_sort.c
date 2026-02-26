#include <stdbool.h>
#include <stdint.h>

#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_path.h"
#include "../include/ft_sort.h"

static void sort_time_(t_array *array);
static void sort_name_(t_array *array);
static int compare_(const t_str *a, const t_str *b);
static int compare_time_(const struct timespec *a, const struct timespec *b);
static void reverse_(t_array *array);

void sort(t_array *array, bool reverse, bool sort_time) {
    ASSERT_NOTNULL(array);

    if (sort_time) {
        sort_time_(array);
    } else {
        sort_name_(array);
    }

    if (reverse && array->len) {
        reverse_(array);
    }
}

static void sort_time_(t_array *array) {
    ASSERT_NOTNULL(array);
    ASSERT_GT(array->len, 0);

    uint64_t size = array->len - 1;
    while (true) {
        bool changed = false;

        for (uint64_t index = 0; index < size; ++index) {
            t_entry *entry_a = array->data[index];
            t_entry *entry_b = array->data[index + 1];

            int cmp = compare_time_(&entry_a->st.st_mtim, &entry_b->st.st_mtim);
            bool should_swap = false;
            if (cmp == 0) {
                should_swap = compare_(entry_a->name, entry_b->name) > 0;
            } else {
                should_swap = cmp < 0;
            }

            if (should_swap) {
                array->data[index] = entry_b;
                array->data[index + 1] = entry_a;
                changed = true;
            }
        }

        if (!changed) {
            break;
        }

        --size;
    }
}

static void sort_name_(t_array *array) {
    ASSERT_NOTNULL(array);
    ASSERT_GT(array->len, 0);

    size_t size = array->len - 1;
    while (true) {
        bool changed = false;

        for (uint64_t index = 0; index < size; ++index) {
            t_entry *entry_a = (t_entry *)array->data[index];
            t_entry *entry_b = (t_entry *)array->data[index + 1];

            int result = compare_(entry_a->name, entry_b->name);
            if (result > 0) {
                array->data[index] = entry_b;
                array->data[index + 1] = entry_a;
                changed = true;
            }
        }

        if (!changed) {
            break;
        }

        --size;
    }
}

static int compare_(const t_str *lhs, const t_str *rhs) {
    ASSERT_NOTNULL(lhs);
    ASSERT_NOTNULL(rhs);

    const unsigned char *a = (const unsigned char *)lhs->str;
    const unsigned char *b = (const unsigned char *)rhs->str;

    if (*a == '\'' || *a == '"') {
        ++a;
    }

    if (*b == '\'' || *b == '"') {
        ++b;
    }

    while (*a && *b) {
        if (*a != *b) {
            return (int)*a - (int)*b;
        }

        ++a;
        ++b;
    }

    return (int)*a - (int)*b;
}

static int compare_time_(const struct timespec *a, const struct timespec *b) {
    ASSERT_NOTNULL(a);
    ASSERT_NOTNULL(b);

    if (a->tv_sec != b->tv_sec) {
        return (a->tv_sec > b->tv_sec) - (a->tv_sec < b->tv_sec);
    }

    return (a->tv_nsec > b->tv_nsec) - (a->tv_nsec < b->tv_nsec);
}

static void reverse_(t_array *array) {
    ASSERT_NOTNULL(array);
    ASSERT_GT(array->len, 0);

    size_t index = 0;
    size_t end = array->len - 1;

    while (index < end) {
        t_entry *tmp = array->data[index];
        array->data[index] = array->data[end];
        array->data[end] = tmp;
        ++index;
        --end;
    }
}
