#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "../include/ft_print.h"
#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_ls.h"
#include "../include/ft_sort.h"
#include "../libft/include/libft.h"
#include "ft_fprintf.h"

static void printer_(const t_args *args, t_array *files, size_t len, bool quoted);
static void single_row_(t_array *files, size_t max_len, bool quoted);
static void multi_row_(t_array *files, size_t max_len, bool quoted, size_t rows);
static size_t get_rows_(size_t file_count, size_t max_len, size_t gap);

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
        printer_(args, path->files, path->max_len, path->quoted);
    }
}

static void printer_(const t_args *args, t_array *files, size_t max_len, bool quoted) {
    ASSERT_(args, "args can not be NULL");
    ASSERT_(files, "files can not be NULL");
    ASSERT_(max_len, "len must be more then 0");

    if (args->time) {
        sort_time(files, args->reverse);
    } else {
        sort_alpha(files);
    }

    size_t gap = quoted ? 3 : 2;
    size_t rows = get_rows_(files->len, max_len, gap);
    if (rows == 1) {
        single_row_(files, max_len, quoted);
    } else {
        multi_row_(files, max_len, quoted, rows);
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

static void multi_row_(t_array *files, size_t max_len, bool quoted, size_t rows) {
    ASSERT_(files, "file can not be NULL");
    ASSERT_(max_len, "max_len can not be NULL");
    ASSERT_(rows, "rows can not be NULL");
    (void)quoted;
    ft_fprintf(STDOUT_FILENO, "rows: %d, cols: %d, files: %d\n", rows, files->len / rows, files->len);
}

static size_t get_rows_(size_t file_count, size_t max_len, size_t gap) {
    ASSERT_(file_count, "file_count should be more then 0");
    ASSERT_(max_len, "len should be more then 0");
    ASSERT_(gap >= 2, "gap should be at least 2");

    size_t col_width = max_len + gap;
    size_t cols = TERM_SIZE / col_width;
    if (cols == 0) {
        cols = 1;
    }

    return (file_count + cols - 1) / cols;
}
