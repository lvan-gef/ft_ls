#include <stdbool.h>
#include <stddef.h>
#include <unistd.h>

#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_ls.h"
#include "../include/ft_sort.h"
#include "../libft/include/libft.h"

static int compare_(const char *a, const char *b);

void sort_alpha(t_array *files) {
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
}

void sort_time(t_array *files, bool reverse) {
    ASSERT_(files, "files can not be NULL");

    (void)files;
    (void)reverse;
}

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

static int compare_(const char *a, const char *b) {
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
