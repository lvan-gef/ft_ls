#include <stdbool.h>
#include <stdint.h>

#include "../include/ft_arena.h"
#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_entry.h"
#include "../include/ft_sort.h"

typedef int (*t_cmp_entry)(const t_entry *a, const t_entry *b);

static void merge_sort_(Arena *arena, t_array *array, t_cmp_entry cmp);
static uint64_t add_capped_(uint64_t lhs, uint64_t rhs, uint64_t cap);
static void merge_(void **data, void **tmp, uint64_t left, uint64_t mid,
                   uint64_t right, t_cmp_entry cmp);
static int cmp_name_entry_(const t_entry *a, const t_entry *b);
static int cmp_time_entry_(const t_entry *a, const t_entry *b);
static int compare_(const t_str *lhs, const t_str *rhs);
static int compare_time_(const struct timespec *a, const struct timespec *b);
static void reverse_(t_array *array);

void sort(Arena *arena, t_array *array, bool reverse, bool sort_time) {
    ASSERT_NOTNULL(array);

    if (array->len > 1) {
        const t_cmp_entry cmp = sort_time ? cmp_time_entry_ : cmp_name_entry_;
        merge_sort_(arena, array, cmp);
    }

    if (reverse && array->len) {
        reverse_(array);
    }
}

static void merge_sort_(Arena *arena, t_array *array, t_cmp_entry cmp) {
    ASSERT_NOTNULL(array);
    ASSERT_NOTNULL(cmp);

    const uint64_t max_len = (uint64_t)(SIZE_MAX / sizeof(void *));
    if (array->len > max_len) {
        return;
    }

    ArenaMark marker = ArenaGetMark(arena);
    void **tmp =
        (void **)ArenaPushNoZero(arena, (size_t)array->len * sizeof(void *));
    if (!tmp) {
        return;
    }

    for (uint64_t width = 1; width < array->len;) {
        uint64_t left = 0;
        while (left < array->len) {
            const uint64_t mid = add_capped_(left, width, array->len);
            const uint64_t right = add_capped_(mid, width, array->len);
            if (mid < right) {
                merge_(array->data, tmp, left, mid, right, cmp);
            }

            const uint64_t step = add_capped_(width, width, array->len);
            left = add_capped_(left, step, array->len);
        }

        if (width >= array->len - width) {
            break;
        }

        width += width;
    }

    ArenaPopToMark(arena, marker);
}

static uint64_t add_capped_(uint64_t lhs, uint64_t rhs, uint64_t cap) {
    if (lhs >= cap || rhs >= cap - lhs) {
        return cap;
    }

    return lhs + rhs;
}

static void merge_(void **data, void **tmp, uint64_t left, uint64_t mid,
                   uint64_t right, t_cmp_entry cmp) {
    uint64_t i = left;
    uint64_t j = mid;
    uint64_t out = left;

    while (i < mid && j < right) {
        const t_entry *a = data[i];
        const t_entry *b = data[j];

        if (cmp(a, b) <= 0) {
            tmp[out++] = data[i++];
        } else {
            tmp[out++] = data[j++];
        }
    }

    while (i < mid) {
        tmp[out++] = data[i++];
    }

    while (j < right) {
        tmp[out++] = data[j++];
    }

    for (uint64_t idx = left; idx < right; ++idx) {
        data[idx] = tmp[idx];
    }
}

static int cmp_name_entry_(const t_entry *a, const t_entry *b) {
    ASSERT_NOTNULL(a);
    ASSERT_NOTNULL(b);

    return compare_(a->name, b->name);
}

static int cmp_time_entry_(const t_entry *a, const t_entry *b) {
    ASSERT_NOTNULL(a);
    ASSERT_NOTNULL(b);

    const int cmp = compare_time_(&a->st.st_mtim, &b->st.st_mtim);
    if (cmp != 0) {
        return -cmp;
    }

    return compare_(a->name, b->name);
}

static int compare_(const t_str *lhs, const t_str *rhs) {
    ASSERT_NOTNULL(lhs);
    ASSERT_NOTNULL(rhs);

    const unsigned char *a = (const unsigned char *)lhs->str;
    const unsigned char *b = (const unsigned char *)rhs->str;

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
