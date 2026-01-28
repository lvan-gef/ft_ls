#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_ls.h"
#include "../include/ft_print.h"
#include "../include/ft_sort.h"
#include "../include/ft_arena.h"

#include "../libft/include/libft.h"
#include "../libft/include/ft_fprintf.h"

static void printer_(const t_args *args, t_path *path);
#ifdef __linux__
static size_t calc_layout_width_(t_array *files, size_t num_cols,
                                 size_t *col_widths, bool quoted);
#endif
static void print_(Arena *arena, t_path *path);
static bool calc_cols_(Arena *arena, t_path *path, size_t **col_widths,
                       size_t *num_cols, size_t *num_rows);

void print_ls(t_args *args) {
    ASSERT_(args, "args can not be NULL");

    t_array *paths = args->paths;
    if (!paths->len) {
        return;
    }

    size_t index = 0;
    while (index < paths->len) {
        t_path *path = paths->data[index];
        ASSERT_(path->max_len, "path->max_len must be more then 0");
        printer_(args, path);
        ++index;
    }
}

static void printer_(const t_args *args, t_path *path) {
    ASSERT_(args, "args can not be NULL");
    ASSERT_(path, "path can not be NULL");

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

    size_t num_cols = 1;
    size_t num_rows = path->files->len;
    size_t *col_widths = NULL;

#ifdef __APPLE__
    size_t colwidth = max_len;
    if (quoted) {
        colwidth += 1;
    }
    colwidth = (colwidth + 8) & ~((size_t)7);

    num_cols = TERM_SIZE / colwidth;
    if (num_cols < 1) {
        num_cols = 1;
    }
    if (num_cols > files->len) {
        num_cols = files->len;
    }
    num_rows = (files->len + num_cols - 1) / num_cols;

    col_widths = Arena(arena, num_cols * sizeof(*col_widths));
    if (!col_widths) {
        return;
    }
    for (size_t c = 0; c < num_cols; c++) {
        col_widths[c] = colwidth - 2;
    }
#endif
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

    for (size_t row = 0; row < num_rows; row++) {
        size_t buf_len = 0;
        size_t cur_pos = 0;

        for (size_t col = 0; col < num_cols; col++) {
            size_t idx = row + col * num_rows;
            if (idx >= path->files->len) {
                break;
            }

            t_file *f = path->files->data[idx];
            bool is_last_col = (col == num_cols - 1) ||
                               (row + (col + 1) * num_rows >= path->files->len);

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
                    if (next_tab <= target_pos) {
                        test_pos = next_tab;
                        num_tabs++;
                    } else {
                        break;
                    }
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
#ifdef __APPLE__
    size_t colwidth = path->len;
    if (path->quoted) {
        colwidth += 1;
    }
    colwidth = (colwidth + 8) & ~((size_t)7);

    *num_cols = TERM_SIZE / colwidth;
    if (*num_cols < 1) {
        *num_cols = 1;
    }
    if (*num_cols > files->len) {
        *num_cols = files->len;
    }
    *num_rows = (files->len + *num_cols - 1) / *num_cols;

    col_widths = malloc(*num_cols * sizeof(*col_widths));
    if (!col_widths) {
        return false;
    }
    for (size_t c = 0; c < *num_cols; c++) {
        col_widths[c] = colwidth - 2;
    }
    return true;
#elif __linux__
    size_t max_cols = path->files->len;
    if (max_cols > TERM_SIZE / 2) {
        max_cols = TERM_SIZE / 2;
    }

    *col_widths = ArenaPush(arena, max_cols * sizeof(*col_widths));
    if (!col_widths) {
        return false;
    }

    for (size_t try_cols = max_cols; try_cols > 1; try_cols--) {
        size_t width = calc_layout_width_(path->files, try_cols, *col_widths,
                                          path->quoted);
        if (width <= TERM_SIZE) {
            *num_cols = try_cols;
            *num_rows = (path->files->len + *num_cols - 1) / *num_cols;
            break;
        }
    }

    (void)calc_layout_width_(path->files, *num_cols, *col_widths, path->quoted);
#else
    ft_fprintf(STDERR_FILENO, "OS is not supported\n");
    return false;
#endif

    return true;
}

#ifdef __linux__
static size_t calc_layout_width_(t_array *files, size_t num_cols,
                                 size_t *col_widths, bool quoted) {
    size_t num_rows = (files->len + num_cols - 1) / num_cols;

    for (size_t c = 0; c < num_cols; ++c) {
        col_widths[c] = 0;
    }

    for (size_t col = 0; col < num_cols; ++col) {
        for (size_t row = 0; row < num_rows; ++row) {
            size_t idx = row + col * num_rows;
            if (idx >= files->len) {
                break;
            }
            t_file *f = files->data[idx];
            size_t len = f->len;
            if (quoted && !(f->filename[0] == '"' || f->filename[0] == '\'')) {
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
            total += 2;
        }
    }

    return total;
}
#endif
