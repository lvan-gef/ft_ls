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

// static int compare_time(const struct timespec *a, const struct timespec *b);
static void reverse_(t_array *files);
static int compare_(const char *a, const char *b);
#if defined(__APPLE__)
static int compare_bsd_(const char *a, const char *b);
#else
static int compare_gnu_(const char *a, const char *b);
#endif

void sort_alpha(t_array *array, bool reverse) {
    ASSERT_(array, "files can not be NULL");
    // ASSERT_(array->len, "files->len must > 0");
    ASSERT_(array->data, "files->data can not be NULL");
    // ASSERT_(array->data[0], "files->data[0] can not be NULL");

    size_t index = 0;
    while (index < array->len) {
        size_t sub_index = index + 1;
        while (sub_index < array->len) {
            switch (array->type) {
                case ARRAY_FILES: {
                    t_file *file_a = (t_file *)array->data[index];
                    const char *filename_a = file_a->name->str;
                    t_file *file_b = (t_file *)array->data[sub_index];
                    const char *filename_b = file_b->name->str;

                    int result = compare_(filename_a, filename_b);
                    if (result > 0) {
                        array->data[sub_index] = file_a;
                        array->data[index] = file_b;
                    }
                    break;
                }
                case ARRAY_PATHS: {
                    t_path *path_a = (t_path *)array->data[index];
                    const char *filename_a = path_a->name->str;
                    t_path *path_b = (t_path *)array->data[sub_index];
                    const char *filename_b = path_b->name->str;

                    int result = compare_(filename_a, filename_b);
                    if (result > 0) {
                        array->data[sub_index] = path_a;
                        array->data[index] = path_b;
                    }
                    break;
                }
            }

            ++sub_index;
        }
        ++index;
    }

    if (reverse) {
        reverse_(array);
    }
}

// void sort_time_files(t_array *files, bool reverse) {
//     ASSERT_(files, "files can not be NULL");
//     ASSERT_(files->len, "files->len must be more then 0");
//     ASSERT_(files->data, "files->data can not be NULL");
//     ASSERT_(files->data[0], "files->data[0] can not be NULL");
//
//     size_t size = files->len - 1;
//     ASSERT_(size < files->len, "size should be less then files->len");
//
//     while (true) {
//         bool changed = false;
//         size_t index = 0;
//
//         while (index < size) {
//             t_file *file_a = files->data[index];
//             t_file *file_b = files->data[index + 1];
//
//             int cmp = compare_time(&file_a->mtime, &file_b->mtime);
//             bool should_swap = false;
//             if (cmp == 0) {
//                 should_swap = compare_(file_a->filename, file_b->filename) > 0;
//             } else {
//                 should_swap = cmp < 0;
//             }
//
//             if (should_swap) {
//                 files->data[index] = file_b;
//                 files->data[index + 1] = file_a;
//                 changed = true;
//             }
//
//             ++index;
//         }
//
//         if (!changed) {
//             break;
//         }
//
//         --size;
//     }
//
//     if (reverse) {
//         reverse_(files);
//     }
// }
//
// void sort_time_paths(t_array *paths, bool reverse) {
//     ASSERT_(paths, "paths can not be NULL");
//     ASSERT_(paths->len, "paths->len must be more then 0");
//     ASSERT_(paths->data, "paths->data can not be NULL");
//     ASSERT_(paths->data[0], "paths->data[0] can not be NULL");
//
//     size_t size = paths->len - 1;
//     ASSERT_(size < paths->len, "size should be less then paths->len");
//
//     while (true) {
//         bool changed = false;
//         size_t index = 0;
//
//         while (index < size) {
//             t_path *path_a = paths->data[index];
//             t_path *path_b = paths->data[index + 1];
//
//             int cmp = compare_time(&path_a->mtime, &path_b->mtime);
//             bool should_swap = false;
//             if (cmp == 0) {
//                 should_swap = compare_(path_a->name, path_b->name) > 0;
//             } else {
//                 should_swap = cmp < 0;
//             }
//
//             if (should_swap) {
//                 paths->data[index] = path_b;
//                 paths->data[index + 1] = path_a;
//                 changed = true;
//             }
//
//             ++index;
//         }
//
//         if (!changed) {
//             break;
//         }
//
//         --size;
//     }
//
//     if (reverse) {
//         reverse_(paths);
//     }
// }
//
// static int compare_time(const struct timespec *a, const struct timespec *b) {
//     ASSERT_(a, "a can not be NULL");
//     ASSERT_(b, "b can not be NULL");
//
//     if (a->tv_sec != b->tv_sec) {
//         return (a->tv_sec > b->tv_sec) - (a->tv_sec < b->tv_sec);
//     }
//
//     return (a->tv_nsec > b->tv_nsec) - (a->tv_nsec < b->tv_nsec);
// }

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
        // ASSERT_(index <= end, "index crossed end");  need to chage it on even
        // it triggert
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
static int compare_gnu_(const char *a, const char *b) {
    ASSERT_(a, "a can not be NULL");
    ASSERT_(b, "b can not be NULL");

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
#endif
