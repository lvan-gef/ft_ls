#include <stdbool.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "./ft_ls.h"
#include "./ft_printer.h"
#include "./ft_printer_helper.h"

typedef struct {
    uint64_t rows;
    uint64_t cols;
} t_map;

static bool init_print_row_(const t_print_request *req);
static bool init_print_lines_(const t_print_request *req);
static bool entries_have_quote_(const t_array *array);
static void calc_cols_(const t_array *array, t_map *map, bool quoted,
                       uint64_t *col_widths, uint64_t max_cols,
                       uint64_t term_size);
static bool calc_width_(const t_array *array, bool quoted, uint64_t num_cols,
                        uint64_t *col_widths, uint64_t term_size);
static bool create_row_(t_str *out, const t_array *array, const t_map *map,
                        const uint64_t *col_widths, bool quoted, bool color);
static bool indent_(t_str *out, uint64_t from, uint64_t to);

bool printer(const t_print_request *req) {
    if (req->list_mode) {
        return printer_list(req);
    }

    if (!req->is_stdout) {
        return init_print_lines_(req);
    }

    return init_print_row_(req);
}

uint64_t get_terminal_width(void) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) {
        return (uint64_t)ws.ws_col;
    }

    return 80;
}

static bool init_print_row_(const t_print_request *req) {
    t_map map = {.cols = 1, .rows = req->entries->len};
    uint64_t *col_widths = NULL;
    const uint64_t max_cols = (req->term_size + SPACE_GAP) / (1 + SPACE_GAP);

    uint64_t width_count =
        req->entries->len < max_cols ? req->entries->len : max_cols;
    if (!width_count) {
        width_count = 1;
    }

    if (width_count > UINT64_MAX / sizeof(*col_widths)) {
        return false;
    }

    col_widths = arena_push(req->arena, width_count * sizeof(*col_widths));
    if (!col_widths) {
        return false;
    }

    const bool quoted = req->quote_padding || entries_have_quote_(req->entries);
    calc_cols_(req->entries, &map, quoted, col_widths, width_count,
               req->term_size);
    if (!put_dir_header(req->buffer, req->dir_header)) {
        arena_clear(req->arena);
        return false;
    }

    const bool ok = create_row_(req->buffer, req->entries, &map, col_widths,
                                quoted, req->color);
    arena_clear(req->arena);
    return ok;
}

static bool init_print_lines_(const t_print_request *req) {
    if (!put_dir_header_raw(req->buffer, req->dir_header)) {
        return false;
    }

    for (uint64_t index = 0; index < req->entries->len; ++index) {
        const t_entry *entry = req->entries->data[index];

        if (!put_entry_name_raw(req->buffer, entry) ||
            !put_mem(req->buffer, "\n", 1)) {
            return false;
        }
    }

    return true;
}

static bool entries_have_quote_(const t_array *array) {
    for (uint64_t index = 0; index < array->len; ++index) {
        const t_entry *entry = array->data[index];
        if (entry->name_scan.quote != '\0') {
            return true;
        }
    }

    return false;
}

static void calc_cols_(const t_array *array, t_map *map, const bool quoted,
                       uint64_t *col_widths, const uint64_t max_cols,
                       const uint64_t term_size) {
    uint64_t width_count = array->len < max_cols ? array->len : max_cols;
    if (!width_count) {
        width_count = 1;
    }

    uint64_t best = 1;
    for (uint64_t cols = width_count; cols > 0; --cols) {
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
                        const uint64_t num_cols, uint64_t *col_widths,
                        const uint64_t term_size) {
    const uint64_t file_count = array->len;
    const uint64_t num_rows = (file_count + num_cols - 1) / num_cols;
    uint64_t line_len = num_cols * MIN_COLUMN_WIDTH;

    for (uint64_t index = 0; index < num_cols; ++index) {
        col_widths[index] = MIN_COLUMN_WIDTH;
    }

    for (uint64_t filesno = 0; filesno < file_count; ++filesno) {
        const uint64_t index = filesno / num_rows;
        const t_entry *entry = array->data[filesno];
        uint64_t real_length = quoted ? entry->name_scan.padded_display_len
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

static bool create_row_(t_str *out, const t_array *array, const t_map *map,
                        const uint64_t *col_widths, const bool quoted,
                        const bool color) {
    const uint64_t files_len = array->len;

    for (uint64_t row = 0; row < map->rows; ++row) {
        uint64_t col = 0;
        uint64_t filesno = row;
        uint64_t pos = 0;

        while (true) {
            const t_entry *entry = array->data[filesno];
            const uint64_t name_length =
                quoted ? entry->name_scan.padded_display_len
                       : entry->name_scan.display_len;
            const uint64_t max_name_length = col_widths[col++];

            if (!put_entry_name(out, entry, quoted, color)) {
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

static bool indent_(t_str *out, uint64_t from, const uint64_t to) {
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
