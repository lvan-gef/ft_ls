#include <stdbool.h>
#include <stdint.h>

#include "../include/ft_array.h"
#include "../include/ft_entry.h"
#include "../include/ft_printer.h"

#include "ft_printer_helper.h"
#include "ft_printer_list.h"
#include "ft_shell_escape.h"

#ifndef TERM_SIZE
#define TERM_SIZE 80
#endif // ifndef !TERM_SIZE

#if TERM_SIZE < 1
#error "TERM_SIZE must be at least 1"
#endif

typedef struct {
    uint64_t rows;
    uint64_t cols;
    uint64_t max;
} t_map;

static uint64_t display_len_(const t_entry *entry, bool quoted);
static bool init_print_row_(const t_print_request *req);
static void calc_cols_(const t_array *array, t_map *map, bool *quoted,
                       uint64_t *col_widths);
static bool calc_width_(const t_array *array, bool quoted, uint64_t num_cols,
                        uint64_t *col_widths);
static bool create_row_(t_str *out, const t_array *array, const t_map *map,
                        const uint64_t *col_widths, bool quoted);
static bool indent_(t_str *out, uint64_t from, uint64_t to);

bool printer(const t_print_request *req) {
    if (!req->list_mode) {
        return init_print_row_(req);
    }

    return printer_list(req);
}

static bool init_print_row_(const t_print_request *req) {
    const uint64_t max_cols = (TERM_SIZE + SPACE_GAP) / (1 + SPACE_GAP);
    t_map map = {.cols = 1,
                 .rows = req->entries->len,
                 .max = req->entries->len < max_cols ? req->entries->len
                                                     : max_cols};

    bool quoted = context_needs_padding(req->quote_padding_context);
    uint64_t col_widths[(TERM_SIZE + SPACE_GAP) / (1 + SPACE_GAP)];
    calc_cols_(req->entries, &map, &quoted, col_widths);

    if (!put_dir_header(req->buffer, req->dir_header)) {
        return false;
    }

    return create_row_(req->buffer, req->entries, &map, col_widths, quoted);
}

static void calc_cols_(const t_array *array, t_map *map, bool *quoted,
                       uint64_t *col_widths) {
    const uint64_t width_count = map->max ? map->max : 1;

    if (!*quoted) {
        for (uint64_t index = 0; index < array->len; ++index) {
            const t_entry *entry = array->data[index];
            t_shell_scan scan;
            shell_scan_str(entry->name, &scan);
            if (scan.quote != '\0') {
                *quoted = true;
                break;
            }
        }
    }

    uint64_t best = 1;
    for (uint64_t cols = width_count; cols > 0; --cols) {
        if (calc_width_(array, *quoted, cols, col_widths)) {
            best = cols;
            break;
        }
    }

    map->cols = best;
    map->rows = (array->len + best - 1) / best;
    (void)calc_width_(array, *quoted, best, col_widths);
}

static bool calc_width_(const t_array *array, const bool quoted,
                        const uint64_t num_cols, uint64_t *col_widths) {
    const uint64_t file_count = array->len;
    const uint64_t num_rows = (file_count + num_cols - 1) / num_cols;
    uint64_t line_len = num_cols * MIN_COLUMN_WIDTH;

    for (uint64_t index = 0; index < num_cols; ++index) {
        col_widths[index] = MIN_COLUMN_WIDTH;
    }

    for (uint64_t filesno = 0; filesno < file_count; ++filesno) {
        const uint64_t index = filesno / num_rows;
        const t_entry *entry = array->data[filesno];
        uint64_t real_length = display_len_(entry, quoted);

        if (index != num_cols - 1) {
            real_length += SPACE_GAP;
        }

        if (col_widths[index] < real_length) {
            line_len += real_length - col_widths[index];
            col_widths[index] = real_length;
            if (line_len >= TERM_SIZE) {
                return false;
            }
        }
    }

    return true;
}

static bool create_row_(t_str *out, const t_array *array, const t_map *map,
                        const uint64_t *col_widths, const bool quoted) {
    const uint64_t files_len = array->len;

    for (uint64_t row = 0; row < map->rows; ++row) {
        uint64_t col = 0;
        uint64_t filesno = row;
        uint64_t pos = 0;

        while (true) {
            const t_entry *entry = array->data[filesno];
            t_shell_scan scan;
            shell_scan_str(entry->name, &scan);
            const uint64_t name_length =
                quoted ? scan.padded_display_len : scan.display_len;
            const uint64_t max_name_length = col_widths[col++];

            if (!put_shell_escaped(out, entry->name, scan.quote, quoted)) {
                return false;
            }

            if (files_len - map->rows <= filesno) {
                break;
            }

            filesno += map->rows;
            if (!indent_(out, pos + name_length, pos + max_name_length)) {
                return false;
            }

            pos += max_name_length;
        }

        if (!put_mem(out, "\n", 1)) {
            return false;
        }
    }

    return true;
}

static uint64_t display_len_(const t_entry *entry, const bool quoted) {
    t_shell_scan scan;
    shell_scan_str(entry->name, &scan);

    if (quoted) {
        return scan.padded_display_len;
    }

    return scan.display_len;
}

static bool indent_(t_str *out, uint64_t from, const uint64_t to) {
    while (from < to) {
        if (TABSIZE != 0 && to / TABSIZE > (from + 1) / TABSIZE) {
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
