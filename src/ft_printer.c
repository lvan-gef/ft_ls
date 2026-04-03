#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_entry.h"
#include "../include/ft_helper.h"
#include "../include/ft_print_list.h"
#include "../include/ft_printer.h"
#include "../include/ft_sort.h"

#include "../libft/include/ft_fprintf.h"
#include "../libft/include/libft.h"

#ifndef TABSIZE
#define TABSIZE UINT64_C(8)
#endif // ifndef TABSIZE //

#ifndef SPACE_GAP
#define SPACE_GAP UINT64_C(2)
#endif /* ifndef SPACE_GAP */

#ifndef MIN_COLUMN_WIDTH
#define MIN_COLUMN_WIDTH UINT64_C(3)
#endif // !MIN_COLUMN_WIDTH

#ifndef OUTPUT_BUFFER_CAP
#define OUTPUT_BUFFER_CAP UINT64_C(4096)
#endif // !OUTPUT_BUFFER_CAP

typedef struct {
    uint64_t rows;
    uint64_t cols;
    uint64_t max;
} t_map;

typedef struct {
    uint64_t len;
    char data[OUTPUT_BUFFER_CAP];
} t_output;

static void init_print_row_(t_array *array, t_entry *dir_entry,
                            bool force_quote_padding);
static uint64_t *calc_cols_(t_array *array, t_map *map, bool quoted);
static uint64_t calc_width_(t_array *array, uint64_t num_cols,
                            uint64_t *col_widths, bool quoted);
static bool print_row_(t_output *out, t_array *array, const t_map *map,
                       const uint64_t *col_widths, bool quoted);
static bool check_quoted_(t_array *array);
static uint64_t display_name_len_(const t_entry *entry, bool pad_unquoted);
static bool indent_(t_output *out, uint64_t from, uint64_t to);
static bool flush_output_(t_output *out);
static bool put_mem_(t_output *out, const char *src, uint64_t len);
static bool put_char_(t_output *out, char c);
static bool put_display_name_(t_output *out, const t_entry *entry,
                              bool pad_unquoted);
static bool write_mem_(void *ctx, const char *src, uint64_t len);

void printer(const t_args *args, t_array *array, t_entry *dir_entry,
             bool print_total, uint64_t min_len_links, uint64_t min_len_sizes,
             bool force_quote_padding) {
    ASSERT_NOTNULL(args);
    ASSERT_NOTNULL(array);

    (void)print_total;
    (void)min_len_links;
    (void)min_len_sizes;

    if (array->len) {
        sort(array, args->reverse, args->time);
    }

    if (args->list) {
        print_list(array, dir_entry, print_total, min_len_links, min_len_sizes,
                   force_quote_padding);
    } else {
        init_print_row_(array, dir_entry, force_quote_padding);
    }
}

static void init_print_row_(t_array *array, t_entry *dir_entry,
                            bool force_quote_padding) {
    ASSERT_NOTNULL(array);

    uint64_t max_cols = (TERM_SIZE + SPACE_GAP) / (1 + SPACE_GAP);
    if (max_cols < 1) {
        max_cols = 1;
    }

    t_map map = {.cols = 1,
                 .rows = array->len,
                 .max = array->len < max_cols ? array->len : max_cols};

    const bool quoted = force_quote_padding || check_quoted_(array);
    const char *err_msg = NULL;
    uint64_t *col_widths = calc_cols_(array, &map, quoted);
    t_output out = {0};

    if (!col_widths) {
        err_msg = "Failed to calc column leng";
        goto done;
    }

    if (dir_entry) {
        if (!put_display_name_(&out, dir_entry, false) ||
            !put_mem_(&out, ":\n", 2)) {
            err_msg = "Failed to write dir header";
            goto done;
        }
    }

    if (!print_row_(&out, array, &map, col_widths, quoted) ||
        !flush_output_(&out)) {
        err_msg = "Failed to write output";
    }

done:
    free(col_widths);
    if (err_msg) {
        ft_fprintf(STDERR_FILENO, "%s\n", err_msg);
    }
}

static bool print_row_(t_output *out, t_array *array, const t_map *map,
                       const uint64_t *col_widths, bool quoted) {
    ASSERT_NOTNULL(out);
    ASSERT_NOTNULL(array);
    ASSERT_NOTNULL(map);
    ASSERT_NOTNULL(col_widths);

    const uint64_t files_len = array->len;
    for (uint64_t row = 0; row < map->rows; ++row) {
        uint64_t col = 0;
        uint64_t filesno = row;
        uint64_t pos = 0;

        while (true) {
            const t_entry *entry = array->data[filesno];
            const uint64_t name_length = display_name_len_(entry, quoted);
            const uint64_t max_name_length = col_widths[col++];

            if (!put_display_name_(out, entry, quoted)) {
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

        if (!put_char_(out, '\n')) {
            return false;
        }
    }

    return true;
}

static uint64_t *calc_cols_(t_array *array, t_map *map, bool quoted) {
    ASSERT_NOTNULL(array);
    ASSERT_NOTNULL(map);

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
    ASSERT_NOTNULL(array);
    ASSERT_NOTNULL(col_widths);
    ASSERT_GT(num_cols, 0);
    const uint64_t num_rows = (array->len + num_cols - 1) / num_cols;
    uint64_t line_len = num_cols * MIN_COLUMN_WIDTH;

    for (uint64_t index = 0; index < num_cols; ++index) {
        col_widths[index] = MIN_COLUMN_WIDTH;
    }

    for (uint64_t filesno = 0; filesno < array->len; ++filesno) {
        const t_entry *entry = array->data[filesno];
        ASSERT_NOTNULL(entry);
        ASSERT_NOTNULL(entry->name);

        const uint64_t idx = filesno / num_rows;
        const uint64_t name_length = display_name_len_(entry, quoted);
        const uint64_t real_length =
            name_length + (idx == num_cols - 1 ? 0 : SPACE_GAP);

        if (col_widths[idx] < real_length) {
            line_len += real_length - col_widths[idx];
            col_widths[idx] = real_length;
            if (line_len >= TERM_SIZE) {
                return line_len;
            }
        }
    }

    return line_len;
}

static bool check_quoted_(t_array *array) {
    ASSERT_NOTNULL(array);

    for (uint64_t index = 0; index < array->len; ++index) {
        const t_entry *entry = array->data[index];
        if (entry->quote != '\0') {
            return true;
        }
    }

    return false;
}

static uint64_t display_name_len_(const t_entry *entry, bool pad_unquoted) {
    ASSERT_NOTNULL(entry);
    ASSERT_NOTNULL(entry->name);

    return shell_display_len(entry->name, entry->quote, pad_unquoted);
}

static bool indent_(t_output *out, uint64_t from, uint64_t to) {
    ASSERT_NOTNULL(out);
    ASSERT_LE(from, to);

    while (from < to) {
        if (TABSIZE != 0 && to / TABSIZE > (from + 1) / TABSIZE) {
            if (!put_char_(out, '\t')) {
                return false;
            }
            from += TABSIZE - from % TABSIZE;
        } else {
            if (!put_char_(out, ' ')) {
                return false;
            }
            ++from;
        }
    }

    return true;
}

static bool flush_output_(t_output *out) {
    ASSERT_NOTNULL(out);

    uint64_t written = 0;
    while (written < out->len) {
        const ssize_t chunk = write(STDOUT_FILENO, out->data + written,
                                    (size_t)(out->len - written));
        if (chunk < 0) {
            return false;
        }
        written += (uint64_t)chunk;
    }

    out->len = 0;
    return true;
}

static bool put_mem_(t_output *out, const char *src, uint64_t len) {
    ASSERT_NOTNULL(out);
    ASSERT_NOTNULL(src);

    while (len) {
        if (out->len == OUTPUT_BUFFER_CAP && !flush_output_(out)) {
            return false;
        }

        const uint64_t avail = OUTPUT_BUFFER_CAP - out->len;
        const uint64_t to_copy = len < avail ? len : avail;
        ft_memcpy(out->data + out->len, src, (size_t)to_copy);
        out->len += to_copy;
        src += to_copy;
        len -= to_copy;
    }

    return true;
}

static bool put_char_(t_output *out, char c) {
    return put_mem_(out, &c, 1);
}

static bool write_mem_(void *ctx, const char *src, uint64_t len) {
    return put_mem_(ctx, src, len);
}

static bool put_display_name_(t_output *out, const t_entry *entry,
                              bool pad_unquoted) {
    ASSERT_NOTNULL(out);
    ASSERT_NOTNULL(entry);
    ASSERT_NOTNULL(entry->name);

    return write_shell_escaped(out, write_mem_, entry->name, entry->quote,
                               pad_unquoted);
}
