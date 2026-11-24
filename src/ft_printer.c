#include <stdbool.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "./ft_ls.h"
#include "./ft_printer.h"
#include "./ft_printer_helper.h"

#ifndef DEFAULT_TERM_SIZE
#define DEFAULT_TERM_SIZE 80
#endif /* ifndef DEFAULT_TERM_SIZE */

typedef struct {
    size_t rows;
    size_t cols;
} t_map;

static bool init_print_row_(const t_print_request *req);
static bool init_print_lines_(const t_print_request *req);
static bool entries_have_quote_(const t_array *array);
static void calc_cols_(const t_array *array, t_map *map, bool quoted,
                       size_t *col_widths, size_t max_cols, size_t term_size);
static bool calc_width_(const t_array *array, bool quoted, size_t num_cols,
                        size_t *col_widths, size_t term_size);
static bool create_row_(const t_print_request *req, const t_map *map,
                        const size_t *col_widths, bool quoted);
static bool indent_(t_str *out, size_t from, size_t to);

bool printer(const t_print_request *req) {
    if (req->list_mode) {
        return printer_list(req);
    }

    if (!req->is_stdout) {
        return init_print_lines_(req);
    }

    return init_print_row_(req);
}

size_t get_terminal_width(void) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) {
        return (size_t)ws.ws_col;
    }

    return DEFAULT_TERM_SIZE;
}

static bool init_print_row_(const t_print_request *req) {
    t_map map = {.cols = 1, .rows = req->entries->len};
    size_t *col_widths = NULL;
    const size_t max_cols = (req->term_size + SPACE_GAP) / (1 + SPACE_GAP);

    size_t width_count =
        req->entries->len < max_cols ? req->entries->len : max_cols;
    if (!width_count) {
        width_count = 1;
    }

    if (width_count > (size_t)PTRDIFF_MAX / sizeof(*col_widths)) {
        return false;
    }

    col_widths = arena_push(req->arena, width_count * sizeof(*col_widths));
    if (!col_widths) {
        return false;
    }

    const bool quoted = req->quote_padding || entries_have_quote_(req->entries);
    calc_cols_(req->entries, &map, quoted, col_widths, width_count,
               req->term_size);
    if (!put_dir_header(req->buffer, req->dir_header, req->is_stdout)) {
        arena_clear(req->arena);
        return false;
    }

    const bool ok = create_row_(req, &map, col_widths, quoted);
    arena_clear(req->arena);
    return ok;
}

static bool init_print_lines_(const t_print_request *req) {
    if (!put_dir_header(req->buffer, req->dir_header, req->is_stdout)) {
        return false;
    }

    for (size_t index = 0; index < req->entries->len; ++index) {
        const t_entry *entry = req->entries->data[index];

        if (!put_entry_name(req->buffer, entry, false, req->color,
                            req->is_stdout) ||
            !put_mem(req->buffer, "\n", 1)) {
            return false;
        }
    }

    return true;
}

static bool entries_have_quote_(const t_array *array) {
    for (size_t index = 0; index < array->len; ++index) {
        const t_entry *entry = array->data[index];
        if (entry->name_scan.quote != '\0') {
            return true;
        }
    }

    return false;
}

static void calc_cols_(const t_array *array, t_map *map, const bool quoted,
                       size_t *col_widths, const size_t max_cols,
                       const size_t term_size) {
    size_t width_count = array->len < max_cols ? array->len : max_cols;
    if (!width_count) {
        width_count = 1;
    }

    size_t best = 1;
    for (size_t cols = width_count; cols > 0; --cols) {
        if (calc_width_(array, quoted, cols, col_widths, term_size)) {
            best = cols;
            break;
        }
    }

    map->cols = best;
    map->rows = (array->len + best - 1) / best;
    (void)calc_width_(array, quoted, best, col_widths, term_size);
}

static bool calc_width_(const t_array *array, const bool quoted,
                        const size_t num_cols, size_t *col_widths,
                        const size_t term_size) {
    const size_t file_count = array->len;
    const size_t num_rows = (file_count + num_cols - 1) / num_cols;
    size_t line_len = num_cols * MIN_COLUMN_WIDTH;

    for (size_t index = 0; index < num_cols; ++index) {
        col_widths[index] = MIN_COLUMN_WIDTH;
    }

    for (size_t filesno = 0; filesno < file_count; ++filesno) {
        const size_t index = filesno / num_rows;
        const t_entry *entry = array->data[filesno];
        size_t real_length = quoted ? entry->name_scan.padded_display_len
                                    : entry->name_scan.display_len;

        if (index != num_cols - 1) {
            real_length += SPACE_GAP;
        }

        if (col_widths[index] < real_length) {
            line_len += real_length - col_widths[index];
            col_widths[index] = real_length;
            if (line_len >= term_size) {
                return false;
            }
        }
    }

    return true;
}

static bool create_row_(const t_print_request *req, const t_map *map,
                        const size_t *col_widths, const bool quoted) {
    const size_t files_len = req->entries->len;

    for (size_t row = 0; row < map->rows; ++row) {
        size_t col = 0;
        size_t filesno = row;
        size_t pos = 0;

        while (true) {
            const t_entry *entry = req->entries->data[filesno];
            const size_t name_length = quoted
                                           ? entry->name_scan.padded_display_len
                                           : entry->name_scan.display_len;
            const size_t max_name_length = col_widths[col++];

            if (!put_entry_name(req->buffer, entry, quoted, req->color, true)) {
                return false;
            }

            if (files_len - map->rows <= filesno) {
                break;
            }

            filesno += map->rows;
            if (!indent_(req->buffer, pos + name_length,
                         pos + max_name_length)) {
                return false;
            }

            pos += max_name_length;
        }

        if (!put_mem(req->buffer, "\n", 1)) {
            return false;
        }
    }

    return true;
}

static bool indent_(t_str *out, size_t from, const size_t to) {
    while (from < to) {
        if (to / TABSIZE > (from + 1) / TABSIZE) {
            if (!put_mem(out, "\t", 1)) {
                return false;
            }

            from += TABSIZE - from % TABSIZE;
        } else {
            if (!put_mem(out, " ", 1)) {
                return false;
            }

            ++from;
        }
    }

    return true;
}
