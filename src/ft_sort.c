#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "../include/ft_array.h"
#include "../include/ft_entry.h"
#include "../include/ft_sort.h"

typedef int (*t_cmp_entry)(const t_entry *a, const t_entry *b);

static void merge_sort_(Arena *arena, t_array *array, t_cmp_entry cmp);
static uint64_t add_capped_(uint64_t lhs, uint64_t rhs, uint64_t cap);
static void merge_(void **data, void **tmp, uint64_t left, uint64_t mid,
                   uint64_t right, t_cmp_entry cmp);
static int cmp_name_entry_(const t_entry *a, const t_entry *b);
static int cmp_name_entry_rev_(const t_entry *a, const t_entry *b);
static int cmp_time_entry_(const t_entry *a, const t_entry *b);
static int cmp_time_entry_rev_(const t_entry *a, const t_entry *b);
static const t_str *entry_name_(const t_entry *entry);
static int compare_(const t_str *lhs, const t_str *rhs);
static int compare_time_(const struct timespec *a, const struct timespec *b);

void sort(Arena *arena, t_array *array, bool reverse, bool sort_time) {
    if (array->len <= 1) {
        return;
    }

    t_cmp_entry cmp;

    if (sort_time) {
        cmp = reverse ? cmp_time_entry_rev_ : cmp_time_entry_;
    } else {
        cmp = reverse ? cmp_name_entry_rev_ : cmp_name_entry_;
    }

    merge_sort_(arena, array, cmp);
}

static void merge_sort_(Arena *arena, t_array *array, t_cmp_entry cmp) {
    const uint64_t max_len = (uint64_t)(SIZE_MAX / sizeof(void *));
    if (array->len > max_len) {
        return;
    }

    void **tmp = (void **)arena_push(arena, (size_t)array->len * sizeof(*tmp));
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

    arena_clear(arena);
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
    return compare_(entry_name_(a), entry_name_(b));
}

static int cmp_name_entry_rev_(const t_entry *a, const t_entry *b) {
    return cmp_name_entry_(b, a);
}

static int cmp_time_entry_(const t_entry *a, const t_entry *b) {
    const int cmp = compare_time_(&a->st.st_mtim, &b->st.st_mtim);
    if (cmp != 0) {
        return -cmp;
    }

    return compare_(entry_name_(a), entry_name_(b));
}

static int cmp_time_entry_rev_(const t_entry *a, const t_entry *b) {
    return cmp_time_entry_(b, a);
}

static const t_str *entry_name_(const t_entry *entry) {
    return entry->name ? entry->name : entry->path;
}

static int compare_(const t_str *lhs, const t_str *rhs) {
    const unsigned char *a = (const unsigned char *)lhs->str;
    const unsigned char *b = (const unsigned char *)rhs->str;
    const uint64_t limit = lhs->len < rhs->len ? lhs->len : rhs->len;

    for (uint64_t index = 0; index < limit; ++index) {
        if (a[index] != b[index]) {
            return (int)a[index] - (int)b[index];
        }
    }

    if (lhs->len == rhs->len) {
        return 0;
    }

    return lhs->len < rhs->len ? -1 : 1;
}

static int compare_time_(const struct timespec *a, const struct timespec *b) {
    if (a->tv_sec != b->tv_sec) {
        return (a->tv_sec > b->tv_sec) - (a->tv_sec < b->tv_sec);
    }

    return (a->tv_nsec > b->tv_nsec) - (a->tv_nsec < b->tv_nsec);
}
