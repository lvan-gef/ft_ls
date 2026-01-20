#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_ls.h"
#include "../include/ft_print.h"
#include "../include/ft_sort.h"
#include "../libft/include/ft_fprintf.h"
#include "../libft/include/libft.h"

static void printer_(const t_args *args, t_array *files, size_t len,
                     bool quoted);
static void single_row_(t_array *files, size_t max_len, bool quoted);
static void multi_row_(t_array *files, size_t max_len, bool quoted);
static size_t get_rows_(size_t file_count, size_t max_len);
static size_t ft_min_(t_array *arr);

void print_ls(t_args *args) {
    ASSERT_(args, "args can not be NULL");

    t_array *paths = args->paths;
    if (paths->len == 0) {
        return;
    } else if (paths->len > 1) {
        size_t index = 0;
        while (index < paths->len) {
            t_path *path = paths->data[index];
            ASSERT_(path->max_len, "path->max_len must be more then 0");
            printer_(args, path->files, path->max_len, path->quoted);
            ++index;
        }
    } else {
        t_path *path = paths->data[0];
        ASSERT_(path->max_len, "path->max_len must be more then 0");
        printer_(args, path->files, path->max_len, path->quoted);
    }
}

static void printer_(const t_args *args, t_array *files, size_t max_len,
                     bool quoted) {
    ASSERT_(args, "args can not be NULL");
    ASSERT_(files, "files can not be NULL");
    ASSERT_(max_len, "len must be more then 0");

    if (args->time) {
        sort_time(files, args->reverse);
    } else {
        sort_alpha(files, args->reverse);
    }

    if (get_rows_(files->len, max_len) == 1) {
        single_row_(files, max_len, quoted);
    } else {
        multi_row_(files, max_len, quoted);
    }
}

static void single_row_(t_array *files, size_t max_len, bool quoted) {
    ASSERT_(files, "files can not be NULL");
    ASSERT_(files->len > 0, "files->len must be > 0");
    ASSERT_(max_len, "max_len must be > 0");

    size_t buf_size = (files->len * (max_len + 8) + quoted);
    char *buf = malloc(buf_size);
    if (!buf) {
        return;
    }
    size_t buf_len = 0;
    size_t cur_pos = 0;
    t_file *f = files->data[0];

    if (quoted && !(f->filename[0] == '"' || f->filename[0] == '\'')) {
        buf[buf_len] = ' ';
        ++buf_len;
        ++cur_pos;
    }

    size_t index = 0;
    while (index < files->len - 1) {
        buf_len += ft_strlcpy(buf + buf_len, f->filename, buf_size - buf_len);
        cur_pos += f->len;

        size_t gap = 2;
        if (quoted) {
            t_file *next = files->data[index + 1];
            char c = next->filename[0];
            gap = (c == '\'' || c == '"') ? 2 : 3;
        }

        size_t target = cur_pos + gap;
        size_t next_tab = ((cur_pos / 8) + 1) * 8;
        bool use_tab = false;

        if ((gap == 2 && next_tab == target) ||
            (gap == 3 && next_tab < target && (target - next_tab) < 2)) {
            use_tab = true;
        }

        if (use_tab) {
            buf[buf_len] = '\t';
            ++buf_len;
            cur_pos = next_tab;
        }

        while (cur_pos < target) {
            buf[buf_len] = ' ';
            ++buf_len;
            ++cur_pos;
        }
        ++index;
        f = files->data[index];
    }

    f = files->data[index];
    buf_len += ft_strlcpy(buf + buf_len, f->filename, buf_size - buf_len);
    buf[buf_len] = '\n';
    ++buf_len;

    (void)write(STDOUT_FILENO, buf, buf_len);
    free(buf);
}

static void multi_row_(t_array *files, size_t max_len, bool quoted) {
    ASSERT_(files, "file can not be NULL");
    ASSERT_(max_len, "max_len can not be NULL");

    size_t min_col_width = ft_min_(files) + 3;
    size_t max_possible_cols = TERM_SIZE / min_col_width;
    max_possible_cols =
        max_possible_cols < files->len
            ? max_possible_cols
            : files->len; //  min(max_possible_cols, file_count);
    ft_fprintf(STDOUT_FILENO, "%d\n", max_possible_cols);
    (void)quoted;
}

static size_t get_rows_(size_t file_count, size_t max_len) {
    ASSERT_(file_count, "file_count should be more then 0");
    ASSERT_(max_len, "max_len should be more then 0");

    const size_t new_len = max_len;
    size_t tmp_count = file_count / 2;

    size_t tmp_width = new_len * tmp_count;
    ASSERT_(tmp_count == 0 || new_len <= SIZE_MAX / tmp_count,
            "overflow in tmp_width");
    tmp_width = new_len * tmp_count;
    size_t rows = 1;

    while (tmp_width > TERM_SIZE) {
        ++rows;
        tmp_count = tmp_count / 2;
        ASSERT_(tmp_count == 0 || new_len <= SIZE_MAX / tmp_count,
                "overflow in tmp_width");
        tmp_width = new_len * tmp_count;
    }

    return rows;
}

static size_t ft_min_(t_array *arr) {
    size_t index = 0;
    size_t min_val = SIZE_MAX;

    while (index < arr->len) {
        t_file *file = arr->data[index];
        if (file->len < min_val) {
            min_val = file->len;
        }
        ++index;
    }

    return min_val;
}
