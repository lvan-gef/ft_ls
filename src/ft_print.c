#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "../include/ft_arena.h"
#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_ls.h"
#include "../include/ft_print.h"
#include "../include/ft_sort.h"

#include "../libft/include/ft_fprintf.h"
#include "../libft/include/libft.h"

static void printer_(const t_args *args, t_path *path);
static void print_(Arena *arena, t_path *path);
static bool calc_cols_(Arena *arena, t_path *path, size_t **col_widths,
                       size_t *num_cols, size_t *num_rows);
#ifdef __linux__
static size_t calc_layout_width_(t_array *files, size_t num_cols,
                                 size_t *col_widths, bool quoted);
#endif

void print_ls(t_args *args) {
    ASSERT_(args, "args can not be NULL");
    ASSERT_(args->paths, "args->paths can not be NULL");
    ASSERT_(args->paths->data, "args->path->data can not be NULL");
    ASSERT_(args->paths->data[0], "args->path->data[0] can not be NULL");

    t_array *paths = args->paths;
    if (!paths->len) {
        return;
    }

    size_t index = 0;
    while (index < paths->len) {
        t_path *path = paths->data[index];
        printer_(args, path);
        ++index;
    }
}

static void printer_(const t_args *args, t_path *path) {
    ASSERT_(args, "args can not be NULL");
    ASSERT_(path, "path can not be NULL");
    ASSERT_(path->files, "path->files can not be NULL");
    ASSERT_(path->files->len, "path->files->len must be > 0");
    ASSERT_(path->files->data, "path->files->data can not be NULL");
    ASSERT_(path->files->data[0], "path->files->data[0] can not be NULL");
    ASSERT_(path->max_len, "path->max_len must be more then 0");
    ASSERT_(path->path, "path->path can not be NULL");
    ASSERT_(*path->path, "*path->path can not be '\\0'");

    if (args->time) {
        sort_time(path->files, args->reverse);
    } else {
        sort_alpha(path->files, args->reverse);
    }

    Arena *arena = ArenaAlloc(4096);
    if (args->list) {
        // TODO: implement list view
    } else {
        print_(arena, path);
    }

    ArenaRelease(arena);
}

static void print_(Arena *arena, t_path *path) {
    ASSERT_(arena, "arena can not be NULL");
    ASSERT_(path, "path can not be NULL");
    ASSERT_(path->files, "path->files can not be NULL");
    ASSERT_(path->files->len, "path->files->len must be > 0");
    ASSERT_(path->files->data, "path->files->data can not be NULL");
    ASSERT_(path->files->data[0], "path->files->data[0] can not be NULL");
    ASSERT_(path->max_len, "path->max_len must be > 0");

    size_t num_cols = 1;
    size_t num_rows = path->files->len;
    size_t *col_widths = NULL;

    if (!calc_cols_(arena, path, &col_widths, &num_cols, &num_rows)) {
        ft_fprintf(STDERR_FILENO, "Failed to alloc memory in arena\n");
        return;
    }

    size_t *col_starts = ArenaPush(arena, (num_cols + 1) * sizeof(*col_starts));
    if (!col_starts) {
        return;
    }

    col_starts[0] = 0;
    for (size_t c = 0; c < num_cols; c++) {
        col_starts[c + 1] = col_starts[c] + col_widths[c] + 2;
    }

    size_t buf_size = TERM_SIZE + 16;
    char *buf = ArenaPush(arena, buf_size);
    if (!buf) {
        ft_fprintf(STDERR_FILENO, "Failed to alloc memory in arena\n");
        return;
    }

    const size_t files_len = path->files->len;
    for (size_t row = 0; row < num_rows; row++) {
        size_t buf_len = 0;
        size_t cur_pos = 0;

        for (size_t col = 0; col < num_cols; col++) {
            size_t idx = row + col * num_rows;
            if (idx >= files_len) {
                break;
            }

            t_file *f = path->files->data[idx];
            bool is_last_col = (col == num_cols - 1) ||
                               (row + (col + 1) * num_rows >= files_len);

            if (path->quoted &&
                !(f->filename[0] == '"' || f->filename[0] == '\'')) {
                buf[buf_len] = ' ';
                ++buf_len;
                ++cur_pos;
            }

            buf_len +=
                ft_strlcpy(buf + buf_len, f->filename, buf_size - buf_len);
            cur_pos += f->len;

            if (!is_last_col) {
                size_t target_pos = col_starts[col + 1];
                size_t gap = target_pos - cur_pos;

                size_t test_pos = cur_pos;
                size_t num_tabs = 0;
                while (test_pos < target_pos) {
                    size_t next_tab = ((test_pos / 8) + 1) * 8;
                    if (next_tab > target_pos) {
                        break;
                    }

                    test_pos = next_tab;
                    num_tabs++;
                }

                size_t spaces_after_tabs = target_pos - test_pos;
                size_t chars_with_tabs = num_tabs + spaces_after_tabs;
                if (num_tabs > 0 && chars_with_tabs < gap) {
                    while (cur_pos < target_pos) {
                        size_t next_tab = ((cur_pos / 8) + 1) * 8;
                        if (next_tab <= target_pos) {
                            buf[buf_len] = '\t';
                            ++buf_len;
                            cur_pos = next_tab;
                        } else {
                            buf[buf_len] = ' ';
                            ++buf_len;
                            ++cur_pos;
                        }
                    }
                } else {
                    while (cur_pos < target_pos) {
                        buf[buf_len] = ' ';
                        ++buf_len;
                        ++cur_pos;
                    }
                }
            }
        }

        buf[buf_len] = '\n';
        ++buf_len;
        if (write(STDOUT_FILENO, buf, buf_len) < 0) {
            break;
        }
    }
}

static bool calc_cols_(Arena *arena, t_path *path, size_t **col_widths,
                       size_t *num_cols, size_t *num_rows) {
    ASSERT_(arena, "arena con not be NULL");
    ASSERT_(path, "path can not be NULL");
    ASSERT_(path->files, "path->files can not be NULL");
    ASSERT_(path->files->len, "path->files->len must be > 0");
    ASSERT_(path->files->data, "path->files->data can not be NULL");
    ASSERT_(path->files->data[0], "path->files->data[0] can not be NULL");
    ASSERT_(col_widths, "col_widths can not be NULL");
    ASSERT_(num_cols, "num_cols can not be NULL");
    ASSERT_(*num_cols, "*num_cols must be > 0");
    ASSERT_(num_rows, "num_rows can not be NULL");
    ASSERT_(*num_rows, "*num_rows must be > 0");

    size_t files_len = path->files->len;
#ifdef __linux__
    size_t max_cols = path->files->len;
    if (max_cols > TERM_SIZE / 2) {
        max_cols = TERM_SIZE / 2;
    }

    *col_widths = ArenaPush(arena, max_cols * sizeof(**col_widths));
    if (!*col_widths) {
        return false;
    }

    for (size_t try_cols = max_cols; try_cols > 1; --try_cols) {
        size_t width = calc_layout_width_(path->files, try_cols, *col_widths,
                                          path->quoted);
        if (width <= TERM_SIZE) {
            *num_cols = try_cols;
            *num_rows = (files_len + *num_cols - 1) / *num_cols;
            break;
        }
    }

    (void)calc_layout_width_(path->files, *num_cols, *col_widths, path->quoted);
#elif __APPLE__
    size_t colwidth = path->max_len;
    if (path->quoted) {
        colwidth += 1;
    }
    colwidth = (colwidth + 8) & ~((size_t)7);

    *num_cols = TERM_SIZE / colwidth;
    if (*num_cols < 1) {
        *num_cols = 1;
    }
    if (*num_cols > files_len) {
        *num_cols = files_len;
    }
    *num_rows = (files_len + *num_cols - 1) / *num_cols;

    *col_widths = ArenaPush(arena, *num_cols * sizeof(**col_widths));
    if (!*col_widths) {
        return false;
    }

    for (size_t c = 0; c < *num_cols; ++c) {
        (*col_widths)[c] = colwidth - 2;
    }
#else
    ft_fprintf(STDERR_FILENO, "OS is not supported\n");
    return false;
#endif

    return true;
}

#ifdef __linux__
static size_t calc_layout_width_(t_array *files, size_t num_cols,
                                 size_t *col_widths, bool quoted) {
    ASSERT_(files, "files can not be NULL");
    ASSERT_(files->len, "files->len must be > 0");
    ASSERT_(files->data, "files->data can not be NULL");
    ASSERT_(num_cols, "num_cols musr be > 0");
    ASSERT_(col_widths, "col_widths can not be NULL");

    size_t num_rows = (files->len + num_cols - 1) / num_cols;
    for (size_t c = 0; c < num_cols; ++c) {
        col_widths[c] = 0;
    }

    for (size_t col = 0; col < num_cols; ++col) {
        for (size_t row = 0; row < num_rows; ++row) {
            ASSERT_(row + col * num_rows >= row + col, "index did overflow");
            size_t index = row + col * num_rows;
            if (index >= files->len) {
                break;
            }
            t_file *f = files->data[index];
            ASSERT_(f, "f can not be NULL");
            ASSERT_(f->len, "f->len can not be NULL");
            size_t len = f->len;
            if (quoted && !(f->filename[0] == '"' || f->filename[0] == '\'')) {
                ASSERT_(len + 1 > len, "len did overflow");
                len += 1;
            }

            if (len > col_widths[col]) {
                col_widths[col] = len;
            }
        }
    }

    size_t total = 0;
    for (size_t c = 0; c < num_cols; c++) {
        total += col_widths[c];
        if (c < num_cols - 1) {
            ASSERT_(total + 2 > total, "total did overflow");
            total += 2;
        }
    }

    return total;
}
#endif
