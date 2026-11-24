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
static void init_print_lines_(const t_print_request *req);
static void calc_cols_(const t_array *array, t_map *map, bool quoted,
                       size_t *col_widths, size_t max_cols, size_t term_size);
static bool calc_width_(const t_array *array, bool quoted, size_t num_cols,
                        size_t *col_widths, size_t term_size);
static void create_row_(const t_print_request *req, const t_map *map,
                        const size_t *col_widths, bool quoted);
static void indent_(t_out *out, size_t from, size_t to);

bool printer(const t_print_request *req) {
    if (req->list_mode) {
        return printer_list(req);
    }

    if (!req->is_stdout) {
        init_print_lines_(req);
        return true;
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

    col_widths = arena_push(req->arena, width_count * sizeof(*col_widths));
    if (!col_widths) {
        return false;
    }

    calc_cols_(req->entries, &map, req->quoted, col_widths, width_count,
               req->term_size);
    put_dir_header(req->buffer, req->dir_header, req->is_stdout);
    create_row_(req, &map, col_widths, req->quoted);
    return true;
}

static void init_print_lines_(const t_print_request *req) {
    put_dir_header(req->buffer, req->dir_header, req->is_stdout);

    for (size_t index = 0; index < req->entries->len; ++index) {
        const t_entry *entry = req->entries->data[index];

        put_entry_name(req->buffer, entry, false, req->color, req->is_stdout);
        out_put(req->buffer, "\n", 1);
    }
}

static void calc_cols_(const t_array *array, t_map *map, const bool quoted,
                       size_t *col_widths, const size_t max_cols,
                       const size_t term_size) {

    size_t cols = max_cols;
    while (!calc_width_(array, quoted, cols, col_widths, term_size) &&
           cols > 1) {
        --cols;
    }

    map->cols = cols;
    map->rows = (array->len + cols - 1) / cols;
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

static void create_row_(const t_print_request *req, const t_map *map,
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

            put_entry_name(req->buffer, entry, quoted, req->color, true);

            if (files_len - map->rows <= filesno) {
                break;
            }

            filesno += map->rows;
            indent_(req->buffer, pos + name_length, pos + max_name_length);

            pos += max_name_length;
        }

        out_put(req->buffer, "\n", 1);
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
