#include <stdbool.h>
#include <stddef.h>

#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_ls.h"
#include "../include/ft_sort.h"

#include "../libft/include/libft.h"

static int comapre(const char *a, const char *b);

void sort_alpha(t_array *files) {
    CUSTOM_ASSERT_(files, "files can not be NULL");

    size_t index = 0;
    while (index < files->len) {
        size_t sub_index = index + 1;
        while (sub_index < files->len) {
            t_file *file_a = (t_file *)files->data[index];
            t_file *file_b = (t_file *)files->data[sub_index];
            int result = comapre(file_a->filename, file_b->filename);
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
    CUSTOM_ASSERT_(files, "files can not be NULL");

    (void)files;
    (void)reverse;
}

static int comapre(const char *a, const char *b) {
    while (*a && *b) {
        while (*a && !ft_isalnum(*a))
            a++;
        while (*b && !ft_isalnum(*b))
            b++;

        if (!*a || !*b)
            break;

        char ca = (char)ft_tolower(*a);
        char cb = (char)ft_tolower(*b);
        if (ca != cb)
            return ca - cb;

        a++;
        b++;
    }

    while (*a && !ft_isalnum(*a))
        a++;
    while (*b && !ft_isalnum(*b))
        b++;

    return *a - *b;
}
