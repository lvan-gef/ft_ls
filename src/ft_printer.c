#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "./ft_array.h"
#include "./ft_ls.h"
#include "./ft_out.h"
#include "./ft_parse.h"
#include "./ft_printer.h"
#include "./ft_shell_escape.h"
#include "./ft_str.h"
#include "./ft_utils.h"

#ifndef DEFAULT_TERM_SIZE
#define DEFAULT_TERM_SIZE 80
#endif /* ifndef DEFAULT_TERM_SIZE */

typedef struct {
    size_t rows;
    size_t cols;
} t_map;

static bool print_columns_(const t_print_request *req);
static void print_lines_(const t_print_request *req);
static void calc_cols_(const t_print_request *req, t_map *map,
                       size_t *col_widths, size_t max_cols);
static bool calc_width_(const t_print_request *req, size_t num_cols,
                        size_t *col_widths);
static void print_rows_(const t_print_request *req, const t_map *map,
                        const size_t *col_widths);
static void indent_(t_out *out, size_t from, size_t to);
static size_t name_width_(const t_entry *entry, bool quoted);
static const char *entry_color_(const t_entry *entry);

bool printer(const t_print_request *req) {
    if (req->ctx->args->list) {
        return printer_list(req);
    }

    if (!req->ctx->is_tty) {
        print_lines_(req);
        return true;
    }

    return print_columns_(req);
}

size_t get_terminal_width(void) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) {
        return (size_t)ws.ws_col;
    }

    return DEFAULT_TERM_SIZE;
}

void put_entry_name(t_out *out, const t_entry *entry, const bool pad_unquoted,
                    const bool color, const bool is_stdout) {
    const t_str *name = entry->name;

    if (!is_stdout) {
        shell_escape_put(out, name, '\0', false);
        return;
    }

    const char *start = color ? entry_color_(entry) : NULL;
    if (start) {
        out_str(out, start);
    }

    shell_escape_put(out, name, entry->name_scan.quote, pad_unquoted);

    if (start) {
        out_put(out, RESET, sizeof(RESET) - 1);
    }
}

void put_dir_header(t_out *out, const t_str *dir_header, const bool is_stdout) {
    if (!dir_header) {
        return;
    }

    if (!is_stdout) {
        shell_escape_put(out, dir_header, '\0', false);
        out_put(out, ":\n", 2);
        return;
    }

    t_shell_scan scan;
    shell_scan_str(dir_header, &scan);
    if (scan.quote == '\0' &&
        ft_memchr(dir_header->str, ':', dir_header->len) != NULL) {
        scan.quote = '\'';
    }

    shell_escape_put(out, dir_header, scan.quote, false);
    out_put(out, ":\n", 2);
}

void put_link_target(t_out *out, const t_str *target, const bool is_stdout) {
    t_shell_scan scan = {0};

    if (is_stdout) {
        shell_scan_str(target, &scan);
    }

    shell_escape_put(out, target, scan.quote, false);
}

static bool print_columns_(const t_print_request *req) {
    const t_print_ctx *ctx = req->ctx;
    t_map map = {.cols = 1, .rows = req->entries->len};
    size_t *col_widths = NULL;
    const size_t max_cols = (ctx->term_size + SPACE_GAP) / (1 + SPACE_GAP);

    size_t width_count =
        req->entries->len < max_cols ? req->entries->len : max_cols;
    if (!width_count) {
        width_count = 1;
    }

    col_widths = arena_push(ctx->arena, width_count * sizeof(*col_widths));
    if (!col_widths) {
        return false;
    }

    calc_cols_(req, &map, col_widths, width_count);
    put_dir_header(ctx->out, req->dir_header, ctx->is_tty);
    print_rows_(req, &map, col_widths);
    return true;
}

static void print_lines_(const t_print_request *req) {
    const t_print_ctx *ctx = req->ctx;

    put_dir_header(ctx->out, req->dir_header, ctx->is_tty);

    for (size_t index = 0; index < req->entries->len; ++index) {
        const t_entry *entry = req->entries->data[index];

        put_entry_name(ctx->out, entry, false, ctx->args->color, ctx->is_tty);
        out_put(ctx->out, "\n", 1);
    }
}

static void calc_cols_(const t_print_request *req, t_map *map,
                       size_t *col_widths, size_t max_cols) {
    size_t cols = max_cols;
    while (!calc_width_(req, cols, col_widths) && cols > 1) {
        --cols;
    }

    map->cols = cols;
    map->rows = (req->entries->len + cols - 1) / cols;
}

static bool calc_width_(const t_print_request *req, size_t num_cols,
                        size_t *col_widths) {
    const t_array *array = req->entries;
    const size_t file_count = array->len;
    const size_t num_rows = (file_count + num_cols - 1) / num_cols;
    size_t line_len = num_cols * MIN_COLUMN_WIDTH;

    for (size_t index = 0; index < num_cols; ++index) {
        col_widths[index] = MIN_COLUMN_WIDTH;
    }

    for (size_t filesno = 0; filesno < file_count; ++filesno) {
        const size_t index = filesno / num_rows;
        const t_entry *entry = array->data[filesno];
        size_t real_length = name_width_(entry, req->quoted);

        if (index != num_cols - 1) {
            real_length += SPACE_GAP;
        }

        if (col_widths[index] < real_length) {
            line_len += real_length - col_widths[index];
            col_widths[index] = real_length;
            if (line_len > req->ctx->term_size) {
                return false;
            }
        }
    }

    return true;
}

static void print_rows_(const t_print_request *req, const t_map *map,
                        const size_t *col_widths) {
    const size_t files_len = req->entries->len;
    t_out *out = req->ctx->out;
    const t_args *args = req->ctx->args;

    for (size_t row = 0; row < map->rows; ++row) {
        size_t col = 0;
        size_t filesno = row;
        size_t pos = 0;

        while (true) {
            const t_entry *entry = req->entries->data[filesno];
            const size_t name_length = name_width_(entry, req->quoted);
            const size_t max_name_length = col_widths[col++];

            put_entry_name(out, entry, req->quoted, args->color, true);
            if (files_len - map->rows <= filesno) {
                break;
            }

            filesno += map->rows;
            indent_(out, pos + name_length, pos + max_name_length);
            pos += max_name_length;
        }

        out_put(out, "\n", 1);
    }
}

static void indent_(t_out *out, size_t from, const size_t to) {
    while (from < to) {
        if (to / TABSIZE > (from + 1) / TABSIZE) {
            out_put(out, "\t", 1);
            from += TABSIZE - from % TABSIZE;
        } else {
            out_put(out, " ", 1);
            ++from;
        }
    }
}

static size_t name_width_(const t_entry *entry, const bool quoted) {
    const t_shell_scan *scan = &entry->name_scan;

    return scan->display_len + (quoted && !scan->quote ? 1 : 0);
}

static const char *entry_color_(const t_entry *entry) {
    const mode_t mode = entry->st.st_mode;

    if (entry->stat_unavailable) {
        return NULL;
    }

    if (S_ISDIR(mode)) {
        return DIRECTORY;
    }

    if (S_ISLNK(mode)) {
        return SYMLINK;
    }

    if (S_ISSOCK(mode)) {
        return SOCKET;
    }

    if (S_ISFIFO(mode)) {
        return FIFO;
    }

    if (S_ISBLK(mode) || S_ISCHR(mode)) {
        return BLOCKCHAR;
    }

    if (S_ISREG(mode) && mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
        return EXECUTABLE;
    }

    return NULL;
}
