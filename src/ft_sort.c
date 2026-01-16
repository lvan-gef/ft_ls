#include <stdbool.h>
#include <stddef.h>

#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_ls.h"
#include "../include/ft_sort.h"

static int comapre_(const char *a, const char *b);

void sort_alpha(t_array *files) {
    ASSERT_(files, "files can not be NULL");
    ASSERT_(files->len, "files->len must be more then 0");

    size_t index = 0;
    while (index < files->len) {
        size_t sub_index = index + 1;
        while (sub_index < files->len) {
            t_file *file_a = (t_file *)files->data[index];
            t_file *file_b = (t_file *)files->data[sub_index];
            int result = comapre_(file_a->filename, file_b->filename);
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

static int comapre_(const char *a, const char *b) {
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
