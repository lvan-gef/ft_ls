#include "../include/ft_print.h"
#include "../include/ft_assert.h"
#include "../include/ft_ls.h"
#include "../include/ft_sort.h"

#include "../libft/include/ft_fprintf.h"
#include "../libft/include/libft.h"

static void printer_(t_args *args, t_list *files, size_t len,
                     size_t file_count);
static void set_padding_(char *padding, size_t len, size_t file_len,
                         size_t rows);
static size_t get_rows_(size_t file_count, size_t len);

void print_ls(t_args *args, t_list *paths) {
    CUSTOM_ASSERT_(args, "args can not be NULL");
    CUSTOM_ASSERT_(paths, "paths can not be NULL");

    const size_t path_count = (size_t)ft_lstsize(paths);
    if (path_count == 0) {
        return;
    } else if (path_count > 1) {
        while (paths) {
            t_path *path = paths->content;
            ft_fprintf(STDOUT_FILENO, "%s:\n", path->path);
            printer_(args, path->files, path->max_len, path->file_count);
            paths = paths->next;
        }
    } else {
        t_path *path = paths->content;
        printer_(args, path->files, path->max_len, path->file_count);
    }
}

// TODO: sort ll by default on alpa, otherwise sort on argument
// TODO: looks like on one row we have 3 spaces except for when we have a name with '' then we have 2 spaces before it
static void printer_(t_args *args, t_list *files, size_t len,
                     size_t file_count) {
    CUSTOM_ASSERT_(args, "args can not be NULL");
    CUSTOM_ASSERT_(files, "files can not be NULL");
    CUSTOM_ASSERT_(len, "len must be more then 0");
    CUSTOM_ASSERT_(file_count, "file_count must be more then 0");

    size_t rows = get_rows_(file_count, len);
    const size_t files_per_row = file_count / rows;

    if (args->time) {
        sort_time(files, args->reverse);
    } else {
        sort_alpha(files, args->reverse);
    }

    size_t file_printed = 0;
    ft_fprintf(STDOUT_FILENO, " ");
    while (files) {
        t_file *file = files->content;

        if (file_printed >= files_per_row) {
            ft_fprintf(STDOUT_FILENO, "\n ");
            file_printed = 0;
        }

        char padding[len + 1];
        char *c = ft_memchr(file->filename, ' ', file->len);
        if (!c) {
            set_padding_(padding, len, file->len, rows);
            ft_fprintf(STDOUT_FILENO, "%s %s", file->filename, padding);
        } else {
            set_padding_(padding, len - 2, file->len, rows);
            ft_fprintf(STDOUT_FILENO, "'%s '%s", file->filename, padding);
        }

        ++file_printed;
        files = files->next;
    }

    ft_fprintf(STDOUT_FILENO, "\n");
}

static size_t get_rows_(size_t file_count, size_t len) {
    CUSTOM_ASSERT_(file_count, "file_count should be more then 0");
    CUSTOM_ASSERT_(len, "len should be more then 0");

    const size_t new_len = len + 1;
    size_t tmp_count = file_count / 2;
    size_t tmp_width = new_len * tmp_count;
    size_t rows = 1;

    while (tmp_width > TERM_SIZE) {
        ++rows;
        tmp_count = tmp_count / 2;
        tmp_width = new_len * tmp_count;
    }

    return rows;
}

static void set_padding_(char *padding, size_t len, size_t file_len,
                         size_t rows) {
    CUSTOM_ASSERT_(len >= file_len, "len should be >= then file_len");
    CUSTOM_ASSERT_(file_len, "file_len should be more then 0");
    CUSTOM_ASSERT_(len, "len should be more then 0");
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
