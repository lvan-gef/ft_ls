#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

#include "../include/ft_str.h"

#include "./ft_entry.h"
#include "./ft_sort.h"

struct timespec;

typedef struct {
    uint64_t left;
    uint64_t mid;
    uint64_t right;
} t_range;

typedef int (*t_cmp_entry)(const t_entry *a, const t_entry *b);

static bool ensure_sort_scratch_(t_sort_scratch *scratch, uint64_t need);
static void merge_sort_(void **tmp, const t_array *array, t_cmp_entry cmp);
static uint64_t add_capped_(uint64_t lhs, uint64_t rhs, uint64_t cap);
static void merge_(void **data, void **tmp, const t_range *range,
                   t_cmp_entry cmp);
static int cmp_name_entry_(const t_entry *a, const t_entry *b);
static int cmp_name_entry_rev_(const t_entry *a, const t_entry *b);
static int cmp_time_entry_(const t_entry *a, const t_entry *b);
static int cmp_time_entry_rev_(const t_entry *a, const t_entry *b);
static int cmp_atime_entry_(const t_entry *a, const t_entry *b);
static int cmp_atime_entry_rev_(const t_entry *a, const t_entry *b);
static t_str *entry_name_(const t_entry *entry);
static int compare_(const t_str *lhs, const t_str *rhs);
static int compare_time_(const struct timespec *a, const struct timespec *b);

bool sort(t_sort_scratch *scratch, const t_array *array, const bool reverse,
          const bool sort_time, const bool access_time) {
    if (array->len <= 1) {
        return true;
    }

    t_cmp_entry cmp;

    if (sort_time && access_time) {
        cmp = reverse ? cmp_atime_entry_rev_ : cmp_atime_entry_;
    }
    else if (sort_time) {
        cmp = reverse ? cmp_time_entry_rev_ : cmp_time_entry_;
    } else {
        cmp = reverse ? cmp_name_entry_rev_ : cmp_name_entry_;
    }

    if (!ensure_sort_scratch_(scratch, array->len)) {
        return false;
    }

    merge_sort_(scratch->data, array, cmp);
    return true;
}

static bool ensure_sort_scratch_(t_sort_scratch *scratch, const uint64_t need) {
    const uint64_t max_len = (uint64_t)(SIZE_MAX / sizeof(void *));
    if (need > max_len) {
        return false;
    }

    if (scratch->cap >= need) {
        return true;
    }

    uint64_t new_cap = scratch->cap ? scratch->cap : 1;
    while (new_cap < need) {
        if (new_cap > max_len / 2) {
            new_cap = need;
            break;
        }

        new_cap *= 2;
    }

    void **new_data = (void **)malloc((size_t)new_cap * sizeof(*new_data));
    if (!new_data) {
        return false;
    }

    free((void *)scratch->data);
    scratch->data = new_data;
    scratch->cap = new_cap;
    return true;
}

static void merge_sort_(void **tmp, const t_array *array,
                        const t_cmp_entry cmp) {
    for (uint64_t width = 1; width < array->len;) {
        uint64_t left = 0;
        while (left < array->len) {
            const uint64_t mid = add_capped_(left, width, array->len);
            t_range range = {.left = left,
                             .mid = mid,
                             .right = add_capped_(mid, width, array->len)};
            if (range.mid < range.right) {
                merge_(array->data, tmp, &range, cmp);
            }

            const uint64_t step = add_capped_(width, width, array->len);
            left = add_capped_(left, step, array->len);
        }

        if (width >= array->len - width) {
            break;
        }

        width += width;
    }
}

static uint64_t add_capped_(const uint64_t lhs, const uint64_t rhs,
                            const uint64_t cap) {
    if (lhs >= cap || rhs >= cap - lhs) {
        return cap;
    }

    return lhs + rhs;
}

static void merge_(void **data, void **tmp, const t_range *range,
                   const t_cmp_entry cmp) {
    uint64_t i = range->left;
    uint64_t j = range->mid;
    uint64_t out = range->left;

    while (i < range->mid && j < range->right) {
        const t_entry *a = data[i];
        const t_entry *b = data[j];

        if (cmp(a, b) <= 0) {
            tmp[out++] = data[i++];
        } else {
            tmp[out++] = data[j++];
        }
    }

    while (i < range->mid) {
        tmp[out++] = data[i++];
    }

    while (j < range->right) {
        tmp[out++] = data[j++];
    }

    for (uint64_t idx = range->left; idx < range->right; ++idx) {
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

static int cmp_atime_entry_(const t_entry *a, const t_entry *b) {
    const int cmp = compare_time_(&a->st.st_atim, &b->st.st_atim);
    if (cmp != 0) {
        return -cmp;
    }

    return compare_(entry_name_(a), entry_name_(b));
}

static int cmp_atime_entry_rev_(const t_entry *a, const t_entry *b) {
    return cmp_atime_entry_(b, a);
}

static t_str *entry_name_(const t_entry *entry) {
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
