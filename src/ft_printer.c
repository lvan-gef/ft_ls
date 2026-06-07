#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#include "../include/ft_array.h"
#include "../include/ft_entry.h"
#include "../include/ft_printer.h"
#include "../include/ft_printer_helper.h"
#include "../include/ft_shell_escape.h"

typedef struct {
    uint64_t rows;
    uint64_t cols;
    uint64_t max;
} t_map;

static uint64_t display_len_(const t_entry *entry, bool quoted);
static void init_print_row_(t_ps *ps);
static void calc_cols_(t_array *array, t_map *map, bool *quoted,
                       uint64_t *col_widths);
static bool calc_width_(t_array *array, bool quoted, uint64_t num_cols,
                        uint64_t *col_widths);
static bool create_row_(t_str *out, t_array *array, const t_map *map,
                        const uint64_t *col_widths, bool quoted);
static bool indent_(t_str *out, uint64_t from, uint64_t to);

void printer(t_ps *ps, bool list_mode) {
    if (list_mode) {
        print_list(ps);
    } else {
        init_print_row_(ps);
    }
}

bool put_dir_header(t_str *out, const t_entry *dir_entry) {
    if (!dir_entry) {
        return true;
    }

    return escaped_out(out, dir_entry->name, dir_entry->quote, false) &&
           put_mem(out, ":\n", 2);
}

static void init_print_row_(t_ps *ps) {
    uint64_t max_cols = (TERM_SIZE + SPACE_GAP) / (1 + SPACE_GAP);
    t_map map = {.cols = 1,
                 .rows = ps->array->len,
                 .max = ps->array->len < max_cols ? ps->array->len : max_cols};

    bool quoted = ps->quote_padding;
    uint64_t col_widths[(TERM_SIZE + SPACE_GAP) / (1 + SPACE_GAP)];
    calc_cols_(ps->array, &map, &quoted, col_widths);

    if (!put_dir_header(ps->buffer, ps->dir_entry)) {
        return;
    }

    if (!create_row_(ps->buffer, ps->array, &map, col_widths, quoted)) {
        return;
    }
}

static bool create_row_(t_str *out, t_array *array, const t_map *map,
                        const uint64_t *col_widths, bool quoted) {
    const uint64_t files_len = array->len;
    for (uint64_t row = 0; row < map->rows; ++row) {
        uint64_t col = 0;
        uint64_t filesno = row;
        uint64_t pos = 0;

        while (true) {
            const t_entry *entry = array->data[filesno];
            const uint64_t name_length = display_len_(entry, quoted);
            const uint64_t max_name_length = col_widths[col++];

            if (!escaped_out(out, entry->name, entry->quote, quoted)) {
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

static void calc_cols_(t_array *array, t_map *map, bool *quoted,
                       uint64_t *col_widths) {
    const uint64_t width_count = map->max ? map->max : 1;

    if (!*quoted) {
        for (uint64_t index = 0; index < array->len; ++index) {
            const t_entry *entry = array->data[index];
            if (entry->quote != '\0') {
                *quoted = true;
                break;
            }
        }
    }

    uint64_t lo = 1;
    uint64_t hi = width_count;
    uint64_t best = 1;
    while (lo <= hi) {
        const uint64_t mid = lo + (hi - lo) / 2;

        if (calc_width_(array, *quoted, mid, col_widths)) {
            best = mid;
            lo = mid + 1;
        } else {
            if (mid == 1) {
                break;
            }
            hi = mid - 1;
        }
    }

    map->cols = best;
    map->rows = (array->len + best - 1) / best;
    (void)calc_width_(array, *quoted, best, col_widths);
}

static bool calc_width_(t_array *array, bool quoted, uint64_t num_cols,
                        uint64_t *col_widths) {
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

static uint64_t display_len_(const t_entry *entry, bool quoted) {
    if (quoted) {
        return entry->padded_display_len;
    }

    return entry->display_len;
}

static bool indent_(t_str *out, uint64_t from, uint64_t to) {
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
