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

static uint64_t display_len_(const t_entry *entry, bool quoted);
static void init_print_row_(t_ps *ps);
static uint64_t *calc_cols_(t_array *array, t_map *map, bool quoted);
static bool calc_width_(t_array *array, uint64_t num_cols,
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

static uint64_t *calc_cols_(t_array *array, t_map *map, bool quoted) {
    const uint64_t width_count = map->max ? map->max : 1;
    uint64_t *col_widths = malloc((size_t)width_count * sizeof(*col_widths));
    if (!col_widths) {
        return NULL;
    }

    uint64_t try_cols = map->max;
    while (try_cols > 1) {
        if (calc_width_(array, try_cols, col_widths, quoted)) {
            map->cols = try_cols;
            map->rows = (array->len + map->cols - 1) / map->cols;
            return col_widths;
        }
        --try_cols;
    }

    (void)calc_width_(array, 1, col_widths, quoted);
    return col_widths;
}

static bool calc_width_(t_array *array, uint64_t num_cols,
                        uint64_t *col_widths, bool quoted) {
    const uint64_t num_rows = (array->len + num_cols - 1) / num_cols;
    uint64_t line_len = num_cols * MIN_COLUMN_WIDTH;

    // Seed each column with the same minimum width GNU ls uses.
    for (uint64_t index = 0; index < num_cols; ++index) {
        col_widths[index] = MIN_COLUMN_WIDTH;
    }

    for (uint64_t filesno = 0; filesno < array->len; ++filesno) {
        const t_entry *entry = array->data[filesno];
        const uint64_t index = filesno / num_rows;
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
