#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "../include/ft_array.h"
#include "../include/ft_entry.h"
#include "../include/ft_printer.h"
#include "../include/ft_printer_helper.h"
#include "../include/ft_shell_escape.h"
#include "../include/ft_sort.h"

#include "../libft/include/ft_fprintf.h"

typedef struct {
    uint64_t rows;
    uint64_t cols;
    uint64_t max;
} t_map;

static void init_print_row_(t_ps *ps);
static uint64_t *calc_cols_(t_array *array, t_map *map, bool quoted);
static uint64_t calc_width_(t_array *array, uint64_t num_cols,
                            uint64_t *col_widths, bool quoted);
static bool create_row_(t_str *out, t_array *array, const t_map *map,
                        const uint64_t *col_widths, bool quoted);
static bool indent_(t_str *out, uint64_t from, uint64_t to);

void printer(t_ps *ps) {
    if (ps->array->len) {
        sort(ps->array, ps->args->reverse, ps->args->time);
    }

    if (ps->args->list) {
        print_list(ps);
    } else {
        init_print_row_(ps);
    }
}

static void init_print_row_(t_ps *ps) {
    uint64_t max_cols = (TERM_SIZE + SPACE_GAP) / (1 + SPACE_GAP);
    t_map map = {.cols = 1,
                 .rows = ps->array->len,
                 .max = ps->array->len < max_cols ? ps->array->len : max_cols};

    const bool quoted = ps->quote_padding || have_quotes(ps->array);
    const char *err_msg = NULL;
    char buffer[OUTPUT_BUFFER_CAP];
    t_str out = {.str = buffer, .cap = sizeof(buffer), .len = 0, .pos = 0};
    out.str[0] = '\0';

    uint64_t *col_widths = calc_cols_(ps->array, &map, quoted);
    if (!col_widths) {
        err_msg = "Failed to calc column leng";
        goto done;
    }

    if (ps->dir_entry) {
        if (!escaped_out(&out, ps->dir_entry->name, ps->dir_entry->quote,
                         false) ||
            !put_mem(&out, ":\n", 2)) {
            err_msg = "Failed to write dir header";
            goto done;
        }
    }

    if (!create_row_(&out, ps->array, &map, col_widths, quoted) ||
        !flush_str(&out)) {
        err_msg = "Failed to write output";
    }

done:
    if (col_widths) {
        free(col_widths);
    }

    if (err_msg) {
        ft_fprintf(STDERR_FILENO, "%s\n", err_msg);
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
            const uint64_t name_length =
                shell_display_len(entry->name, entry->quote, quoted);
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

static uint64_t *calc_cols_(t_array *array, t_map *map, bool quoted) {
    const uint64_t max_cols = TERM_SIZE / MIN_COLUMN_WIDTH;
    if (map->max > max_cols) {
        map->max = max_cols;
    }

    const uint64_t width_count = map->max ? map->max : 1;
    uint64_t *col_widths = malloc((size_t)width_count * sizeof(*col_widths));
    if (!col_widths) {
        return NULL;
    }

    uint64_t try_cols = map->max;
    while (try_cols > 1) {
        const uint64_t width = calc_width_(array, try_cols, col_widths, quoted);
        if (width < TERM_SIZE) {
            map->cols = try_cols;
            map->rows = (array->len + map->cols - 1) / map->cols;
            return col_widths;
        }
        --try_cols;
    }

    (void)calc_width_(array, 1, col_widths, quoted);
    return col_widths;
}

static uint64_t calc_width_(t_array *array, uint64_t num_cols,
                            uint64_t *col_widths, bool quoted) {
    const uint64_t num_rows = (array->len + num_cols - 1) / num_cols;
    uint64_t line_len = 0;

    for (uint64_t index = 0; index < num_cols; ++index) {
        col_widths[index] = 0;
    }

    for (uint64_t filesno = 0; filesno < array->len; ++filesno) {
        const t_entry *entry = array->data[filesno];
        const uint64_t idx = filesno / num_rows;
        const uint64_t name_length =
            shell_display_len(entry->name, entry->quote, quoted);

        if (col_widths[idx] < name_length) {
            col_widths[idx] = name_length;
        }
    }

    for (uint64_t index = 0; index < num_cols; ++index) {
        uint64_t width = col_widths[index];

        if (index != num_cols - 1) {
            if (width < MIN_COLUMN_WIDTH)
                width = MIN_COLUMN_WIDTH;
            width += SPACE_GAP;
        }

        col_widths[index] = width;
        line_len += width;
    }

    return line_len;
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
