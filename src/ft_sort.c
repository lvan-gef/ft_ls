#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

#include "./ft_array.h"
#include "./ft_ls.h"
#include "./ft_parse.h"
#include "./ft_sort.h"
#include "./ft_str.h"

struct timespec;

typedef struct {
    size_t left;
    size_t mid;
    size_t right;
} t_range;

typedef struct {
    bool reverse;
    bool sort_time;
    bool access_time;
} t_sort_opts;

static void merge_sort_(void **buffer, const t_array *array,
                        const t_sort_opts *sort_opts);
static void merge_(void **data, void **buffer, const t_range *range,
                   const t_sort_opts *sort_opts);
static int compare_(const t_str *lhs, const t_str *rhs);
static int compare_time_(const struct timespec *a, const struct timespec *b);
static int cmp_entry_(const t_entry *a, const t_entry *b,
                      const t_sort_opts *opts);
static size_t min_(size_t a, size_t b);

bool sort(const t_array *array, const t_args *args, const bool reverse) {
    if (args->unsort || array->len <= 1) {
        return true;
    }

    void **buffer = (void **)malloc(array->len * sizeof(*buffer));
    if (!buffer) {
        return false;
    }

    const t_sort_opts sort_opts = {.reverse = reverse,
                                   .sort_time = args->sort_time,
                                   .access_time = args->access_time};
    merge_sort_(buffer, array, &sort_opts);
    free((void *)buffer);
    return true;
}

static void merge_sort_(void **buffer, const t_array *array,
                        const t_sort_opts *sort_opts) {
    for (size_t width = 1; width < array->len; width *= 2) {
        for (size_t left = 0; left < array->len; left += 2 * width) {
            const t_range range = {.left = left,
                                   .mid = min_(left + width, array->len),
                                   .right = min_(left + 2 * width, array->len)};

            if (range.mid < range.right) {
                merge_(array->data, buffer, &range, sort_opts);
            }
        }
    }
}

static void merge_(void **data, void **buffer, const t_range *range,
                   const t_sort_opts *sort_opts) {
    size_t i = range->left;
    size_t j = range->mid;
    size_t out = range->left;

    while (i < range->mid && j < range->right) {
        const t_entry *a = data[i];
        const t_entry *b = data[j];

        if (cmp_entry_(a, b, sort_opts) <= 0) {
            buffer[out++] = data[i++];
        } else {
            buffer[out++] = data[j++];
        }
    }

    while (i < range->mid) {
        buffer[out++] = data[i++];
    }

    while (j < range->right) {
        buffer[out++] = data[j++];
    }

    for (size_t idx = range->left; idx < range->right; ++idx) {
        data[idx] = buffer[idx];
    }
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

static int cmp_entry_(const t_entry *a, const t_entry *b,
                      const t_sort_opts *opts) {
    int r = 0;

    if (opts->sort_time) {
        const struct timespec *ta =
            opts->access_time ? &a->st.st_atim : &a->st.st_mtim;
        const struct timespec *tb =
            opts->access_time ? &b->st.st_atim : &b->st.st_mtim;
        r = -compare_time_(ta, tb);
    }

    if (r == 0) {
        r = compare_(a->name, b->name);
    }

    return opts->reverse ? -r : r;
}

static size_t min_(const size_t a, const size_t b) {
    return a < b ? a : b;
}
