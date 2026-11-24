#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

#include "./ft_ls.h"
#include "./ft_sort.h"
#include "./ft_str.h"

struct timespec;

typedef struct {
    size_t left;
    size_t mid;
    size_t right;
} t_range;

typedef int (*t_cmp_entry)(const t_entry *a, const t_entry *b);

static bool ensure_sort_scratch_(t_sort_scratch *scratch, size_t need);
static void merge_sort_(void **tmp, const t_array *array, t_cmp_entry cmp);
static size_t get_cap_(size_t lhs, size_t rhs, size_t cap);
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
    } else if (sort_time) {
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

static bool ensure_sort_scratch_(t_sort_scratch *scratch, const size_t need) {
    const size_t max_len = PTRDIFF_MAX / sizeof(void *);
    if (need > max_len) {
        return false;
    }

    if (scratch->cap >= need) {
        return true;
    }

    size_t new_cap = scratch->cap ? scratch->cap : 1;
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
    for (size_t width = 1; width < array->len;) {
        size_t left = 0;
        while (left < array->len) {
            const size_t mid = get_cap_(left, width, array->len);
            t_range range = {.left = left,
                             .mid = mid,
                             .right = get_cap_(mid, width, array->len)};
            if (range.mid < range.right) {
                merge_(array->data, tmp, &range, cmp);
            }

            const size_t step = get_cap_(width, width, array->len);
            left = get_cap_(left, step, array->len);
        }

        if (width >= array->len - width) {
            break;
        }

        width += width;
    }
}

static size_t get_cap_(const size_t lhs, const size_t rhs, const size_t cap) {
    if (lhs >= cap || rhs >= cap - lhs) {
        return cap;
    }

    return lhs + rhs;
}

static void merge_(void **data, void **tmp, const t_range *range,
                   const t_cmp_entry cmp) {
    size_t i = range->left;
    size_t j = range->mid;
    size_t out = range->left;

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

    for (size_t idx = range->left; idx < range->right; ++idx) {
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
    const size_t limit = lhs->len < rhs->len ? lhs->len : rhs->len;

    for (size_t index = 0; index < limit; ++index) {
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
