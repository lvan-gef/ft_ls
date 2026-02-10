#include <stdbool.h>
#include <stdint.h>

#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_path.h"
#include "../include/ft_sort.h"

#include "../libft/include/libft.h"

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

    size_t size = array->len - 1;

    while (true) {
        bool changed = false;
        size_t index = 0;

        while (index < size) {
            t_entry *entry_a = array->data[index];
            t_entry *entry_b = array->data[index + 1];

#if defined (__linux__)
            int cmp = compare_time_(&entry_a->st.st_mtim, &entry_b->st.st_mtim);
#else
            int cmp = compare_time_(&entry_a->st.st_mtimespec, &entry_b->st.st_mtimespec);
#endif
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

            ++index;
        }

        if (!changed) {
            break;
        }

        --size;
    }
}

static void sort_name_(t_array *array) {
    ASSERT_NOTNULL(array);

    uint64_t index = 0;
    while (index < array->len) {
        size_t sub_index = index + 1;
        while (sub_index < array->len) {
            t_entry *entry_a = (t_entry *)array->data[index];
            t_entry *entry_b = (t_entry *)array->data[sub_index];
            int result = compare_(entry_a->name, entry_b->name);
            if (result > 0) {
                array->data[sub_index] = entry_a;
                array->data[index] = entry_b;
            }
            ++sub_index;
        }
        ++index;
    }
}

static int compare_(const t_str *lhs, const t_str *rhs) {
    ASSERT_NOTNULL(lhs);
    ASSERT_NOTNULL(rhs);

    if (lhs->len > rhs->len) {
        return 1;
    }

    const char *a = lhs->str;
    const char *b = rhs->str;
    if (*a == '\'' || *a == '"') {
        ++a;
    }

    if (*b == '\'' || *b == '"') {
        ++b;
    }

    const char *a_start = a;
    const char *b_start = b;
    while (*a || *b) {
        while (*a && !ft_isalpha(*a) && !ft_isdigit(*a)) {
            ++a;
        }
        while (*b && !ft_isalpha(*b) && !ft_isdigit(*b)) {
            ++b;
        }

        if (!*a || !*b) {
            break;
        }

        int va;
        int vb;
        if (ft_isdigit(*a)) {
            va = *a - '0';
        } else {
            va = ft_tolower(*a) - 'a' + 10;
        }
        if (ft_isdigit(*b)) {
            vb = *b - '0';
        } else {
            vb = ft_tolower(*b) - 'a' + 10;
        }

        if (va != vb) {
            return va - vb;
        }

        ++a;
        ++b;
    }

    while (*a && !ft_isalpha(*a) && !ft_isdigit(*a)) {
        ++a;
    }
    while (*b && !ft_isalpha(*b) && !ft_isdigit(*b)) {
        ++b;
    }

    if (*a || *b) {
        return *a ? 1 : -1;
    }

    a = a_start;
    b = b_start;
    while (*a && *b) {
        if (*a != *b) {
            if (ft_isalpha(*a) && ft_isalpha(*b) &&
                ft_tolower(*a) == ft_tolower(*b)) {
                int a_lower = (*a >= 'a' && *a <= 'z');
                int b_lower = (*b >= 'a' && *b <= 'z');
                return b_lower - a_lower;
            }
            return (unsigned char)*a - (unsigned char)*b;
        }
        ++a;
        ++b;
    }
    return (unsigned char)*a - (unsigned char)*b;
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
