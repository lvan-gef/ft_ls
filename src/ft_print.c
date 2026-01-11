#include "../include/ft_print.h"
#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_ls.h"
#include "../include/ft_sort.h"

#include "../libft/include/ft_fprintf.h"
#include "../libft/include/libft.h"

static void printer_(const t_args *args, t_array *files, size_t len);
static void set_padding_(char *padding, size_t len, size_t file_len,
                         size_t rows);
static size_t get_rows_(size_t file_count, size_t len);
static char *need_quote_(const char *str);

void print_ls(t_args *args) {
    CUSTOM_ASSERT_(args, "args can not be NULL");

    t_array *paths = args->paths;
    if (paths->len == 0) {
        return;
    } else if (paths->len > 1) {
        size_t index = 0;
        while (index < paths->len) {
            t_path *path = paths->data[index];
            CUSTOM_ASSERT_(path->max_len, "path->max_len must be more then 0");
            printer_(args, path->files, path->max_len);
            ++index;
        }
    } else {
        t_path *path = paths->data[0];
        printer_(args, path->files, path->max_len);
    }
}

// TODO: when multi row then print up -> down left -> rigth
static void printer_(const t_args *args, t_array *files, size_t len) {
    CUSTOM_ASSERT_(args, "args can not be NULL");
    CUSTOM_ASSERT_(files, "files can not be NULL");
    CUSTOM_ASSERT_(len, "len must be more then 0");

    size_t rows = get_rows_(files->len, len);
    const size_t files_per_row = files->len / rows;

    if (args->time) {
        sort_time(files, args->reverse);
    } else {
        sort_alpha(files);
    }

    size_t file_printed = 0;
    // ft_fprintf(STDOUT_FILENO, " ");
    size_t index = 0;
    while (index < files->len) {
        t_file *file = files->data[index];

        if (file_printed >= files_per_row) {
            ft_fprintf(STDOUT_FILENO, "\n ");
            file_printed = 0;
        }

        char padding[len + 1];
        const char *c = need_quote_(file->filename);

        const char *next_is_quoted = false;
        if (index + 1 < files->len) {
            const t_file *next = files->data[index + 1];
            next_is_quoted = need_quote_(next->filename);
        }

        if (!c) {
            set_padding_(padding, len, file->len, rows);
            if (next_is_quoted) {
                ft_fprintf(STDOUT_FILENO, "%s%s", file->filename, padding);
            } else {
                ft_fprintf(STDOUT_FILENO, "%s %s", file->filename, padding);
            }
        } else {
            set_padding_(padding, len - 2, file->len, rows);
            char *quote = "'";
            if (*c == '\'') {
                quote = "\"";
            }

            if (next_is_quoted) {
                ft_fprintf(STDOUT_FILENO, "%s%s%s%s", quote, file->filename, quote, padding);
            } else {
                ft_fprintf(STDOUT_FILENO, "%s%s%s %s", quote, file->filename, quote, padding);
            }
        }

        ++file_printed;
        ++index;
    }

    ft_fprintf(STDOUT_FILENO, "\n");
}

static size_t get_rows_(size_t file_count, size_t len) {
    CUSTOM_ASSERT_(file_count, "file_count should be more then 0");
    CUSTOM_ASSERT_(len, "len should be more then 0");

    const size_t new_len = len + 1;
    size_t tmp_count = file_count / 2;
    CUSTOM_ASSERT_(tmp_count < file_count, "tmp_count did underflow");

    size_t tmp_width = new_len * tmp_count;  // make a assert for it??
    size_t rows = 1;

    // make asserts in the while loop??
    while (tmp_width > TERM_SIZE) {
        ++rows;
        tmp_count = tmp_count / 2;
        tmp_width = new_len * tmp_count;
    }

    return rows;
}

static void set_padding_(char *padding, size_t len, size_t file_len,
                         size_t rows) {
    CUSTOM_ASSERT_(file_len, "file_len should be more then 0");
    CUSTOM_ASSERT_(len, "len should be more then 0");
    CUSTOM_ASSERT_(len >= file_len, "len should be >= then file_len");
    CUSTOM_ASSERT_(rows, "rows should be more then 0");

    size_t index = 0;
    size_t differ = 2;
    if (rows > 1) {
        differ = len - file_len;
    }

    while (index < differ) {
        padding[index] = ' ';
        ++index;
    }

    padding[index] = '\0';
}

static char *need_quote_(const char *str) {
    size_t index = 0;

    while (str[index]) {
        char *c = ft_strchr(" '", str[index]);
        if (c) {
            return c;
        }

        ++index;
    }

    return NULL;
}
