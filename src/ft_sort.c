#include <stdbool.h>
#include <stddef.h>
#include <unistd.h>

#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_ls.h"
#include "../include/ft_sort.h"

#if defined(__linux__)
#include "../libft/include/libft.h"
#endif

static int compare_time(const struct timespec *a, const struct timespec *b);
static void reverse_(t_array *files);
static int compare_(const char *a, const char *b);
#if defined(__APPLE__)
static int compare_bsd_(const char *a, const char *b);
#else
static int compare_gnu_(const char *a, const char *b);
static int get_priority(char c);
#endif

void sort_alpha(t_array *files, bool reverse) {
    ASSERT_(files, "files can not be NULL");
    ASSERT_(files->len, "files->len must be more then 0");

    size_t index = 0;
    while (index < files->len) {
        size_t sub_index = index + 1;
        while (sub_index < files->len) {
            t_file *file_a = (t_file *)files->data[index];
            t_file *file_b = (t_file *)files->data[sub_index];
            int result = compare_(file_a->filename, file_b->filename);
            if (result > 0) {
                files->data[sub_index] = file_a;
                files->data[index] = file_b;
            }
            ++sub_index;
        }
        ++index;
    }

    if (reverse) {
        reverse_(files);
    }
}

void sort_time(t_array *files, bool reverse) {
    ASSERT_(files, "files can not be NULL");
    ASSERT_(files->len, "files->len must be more then 0");

    size_t size = files->len - 1;
    ASSERT_(size < files->len, "size should be less then files->len");

    while (true) {
        bool changed = false;
        size_t index = 0;

        while (index < size) {
            t_file *file_a = files->data[index];
            t_file *file_b = files->data[index + 1];

            int cmp = compare_time(&file_a->mtime, &file_b->mtime);
            bool should_swap = false;
            if (cmp == 0) {
                should_swap = compare_(file_a->filename, file_b->filename) > 0;
            } else {
                should_swap = cmp < 0;
            }

            if (should_swap) {
                files->data[index] = file_b;
                files->data[index + 1] = file_a;
                changed = true;
            }

            ++index;
        }

        if (!changed) {
            break;
        }

        --size;
    }

    if (reverse) {
        reverse_(files);
    }
}

static int compare_time(const struct timespec *a, const struct timespec *b) {
    ASSERT_(a, "a can not be NULL");
    ASSERT_(b, "b can not be NULL");

    if (a->tv_sec != b->tv_sec) {
        return (a->tv_sec > b->tv_sec) - (a->tv_sec < b->tv_sec);
    }

    return (a->tv_nsec > b->tv_nsec) - (a->tv_nsec < b->tv_nsec);
}

static void reverse_(t_array *files) {
    ASSERT_(files, "files can not be NULL");
    ASSERT_(files->len, "files->len must be more then 0");

    size_t index = 0;
    size_t end = files->len - 1;

    while (index < end) {
        t_file *tmp = files->data[index];
        files->data[index] = files->data[end];
        files->data[end] = tmp;
        ++index;
        --end;
        ASSERT_(index <= end, "index crossed end");
    }
}

static int compare_(const char *a, const char *b) {
#if defined(__APPLE__)
    int result = compare_bsd_(a, b);
#else
    int result = compare_gnu_(a, b);
#endif
    return result;
}

#if defined(__APPLE__)
static int compare_bsd_(const char *a, const char *b) {
    ASSERT_(a, "a can not be NULL");
    ASSERT_(*a, "*a can not be '\\0'");
    ASSERT_(b, "b can not be NULL");
    ASSERT_(*b, "*b can not be '\\0'");

    if (*a == '\'' || *a == '"') {
        ++a;
    }
    if (*b == '\'' || *b == '"') {
        ++b;
    }

    while (*a && *b && *a == *b) {
        ++a;
        ++b;
    }

    return (unsigned char)*a - (unsigned char)*b;
}
#else
static int get_priority(char c) {
    ASSERT_(c, "c can not be '\\0'");
    if (ft_isalpha(c)) {
        return 0;
    }

    if (ft_isdigit(c)) {
        return 1;
    }

    return 2;
}

static int compare_gnu_(const char *a, const char *b) {
    ASSERT_(a, "a can not be NULL");
    ASSERT_(*a, "*a can not be '\\0'");
    ASSERT_(b, "b can not be NULL");
    ASSERT_(*b, "*b can not be '\\0'");

    if (*a == '\'' || *a == '"') {
        ++a;
    }

    if (*b == '\'' || *b == '"') {
        ++b;
    }

    const char *a_clean = a;
    const char *b_clean = b;
    while (*a || *b) {
        while (*a && !ft_isalpha(*a)) {
            ++a;
        }

        while (*b && !ft_isalpha(*b)) {
            ++b;
        }

        if (!*a || !*b) {
            break;
        }

        if (ft_tolower(*a) != ft_tolower(*b)) {
            return ft_tolower(*a) - ft_tolower(*b);
        }

        ++a;
        ++b;
    }

    while (*a && !ft_isalpha(*a)) {
        ++a;
    }

    while (*b && !ft_isalpha(*b)) {
        ++b;
    }

    if (*a || *b) {
        return ft_isalpha(*a) ? 1 : -1;
    }

    a = a_clean;
    b = b_clean;
    while (*a && *b) {
        int pa = get_priority(*a);
        int pb = get_priority(*b);

        if (pa != pb) {
            return pa - pb;
        }

        if (ft_tolower(*a) != ft_tolower(*b)) {
            return ft_tolower(*a) - ft_tolower(*b);
        }

        ++a;
        ++b;
    }

    return *a - *b;
}
#endif
