#include <stdbool.h>
#include <stdint.h>

#include "../include/ft_arena.h"
#include "../include/ft_array.h"
#include "../include/ft_entry.h"
#include "../include/ft_printer.h"
#include "../include/ft_printer_helper.h"
#include "../include/ft_shell_escape.h"

#include "../libft/include/libft.h"

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

typedef struct {
    uint64_t total;
    uint64_t max_len_links;
    uint64_t max_len_sizes;
    uint64_t max_len_perm;
    bool have_quote;
} t_list_stats;

static bool prepare_list_infos_(Arena *arena, const t_array *entries);
static void clear_list_infos_(const t_array *entries);
static bool context_needs_padding_(const t_array *context);
static void apply_list_width_context_(const t_array *context,
                                      t_list_stats *sizes);
static bool put_dir_header_(t_str *out, const t_str *dir_header);
static uint64_t display_len_(const t_entry *entry, bool quoted);
static bool init_print_row_(const t_print_request *req);
static void calc_cols_(const t_array *array, t_map *map, bool *quoted,
                       uint64_t *col_widths);
static bool calc_width_(const t_array *array, bool quoted, uint64_t num_cols,
                        uint64_t *col_widths);
static bool create_row_(t_str *out, const t_array *array, const t_map *map,
                        const uint64_t *col_widths, bool quoted);
static bool indent_(t_str *out, uint64_t from, uint64_t to);
static void update_list_stats_(t_list_stats *stats, const t_entry *entry);
static bool print_list_(const t_print_request *req);
static bool left_pad_(t_str *out, uint64_t src_len, uint64_t max_size);
static bool put_uint_(t_str *out, uint64_t value);
static void collect_list_stats_(const t_array *entries, t_list_stats *stats);
static bool print_list_rows_(t_str *out, const t_array *array,
                             const t_list_stats *sizes);

bool printer(const t_print_request *req) {
    if (!req->list_mode) {
        return init_print_row_(req);
    }

    const Arena_Mark mark = arena_get_mark(req->arena);
    bool ok = true;
    if (!prepare_list_infos_(req->arena, req->entries) ||
        !prepare_list_infos_(req->arena, req->list_width_context)) {
        ok = false;
        goto cleanup;
    }

    ok = print_list_(req);
cleanup:
    clear_list_infos_(req->entries);
    clear_list_infos_(req->list_width_context);
    arena_pop_to_mark(req->arena, mark);
    return ok;
}

static bool prepare_list_infos_(Arena *arena, const t_array *entries) {
    if (!entries) {
        return true;
    }

    for (uint64_t index = 0; index < entries->len; ++index) {
        t_entry *entry = entries->data[index];

        entry->info = NULL;
        if (!fill_file_info(arena, entry)) {
            return false;
        }
    }

    return true;
}

static void clear_list_infos_(const t_array *entries) {
    if (!entries) {
        return;
    }

    for (uint64_t index = 0; index < entries->len; ++index) {
        t_entry *entry = entries->data[index];
        entry->info = NULL;
    }
}

static bool context_needs_padding_(const t_array *context) {
    if (!context) {
        return false;
    }

    for (uint64_t index = 0; index < context->len; ++index) {
        const t_str *str = context->data[index];
        if (!str) {
            continue;
        }

        t_shell_scan scan;
        shell_scan_str(str, &scan);
        if (scan.quote != '\0') {
            return true;
        }
    }

    return false;
}

static void apply_list_width_context_(const t_array *context,
                                      t_list_stats *sizes) {
    if (!context) {
        return;
    }

    for (uint64_t index = 0; index < context->len; ++index) {
        const t_entry *entry = context->data[index];
        if (!entry || !entry->info) {
            continue;
        }

        if (entry->info->links->len > sizes->max_len_links) {
            sizes->max_len_links = entry->info->links->len;
        }

        if (entry->info->size->len > sizes->max_len_sizes) {
            sizes->max_len_sizes = entry->info->size->len;
        }
    }
}

static bool put_dir_header_(t_str *out, const t_str *dir_header) {
    if (!dir_header) {
        return true;
    }

    t_shell_scan scan;
    shell_scan_str(dir_header, &scan);
    if (scan.quote == '\0' && ft_memchr(dir_header->str + dir_header->pos, ':',
                                        (size_t)dir_header->len) != NULL) {
        scan.quote = '\'';
    }

    return escaped_out(out, dir_header, scan.quote, false) &&
           put_mem(out, ":\n", 2);
}

static bool init_print_row_(const t_print_request *req) {
    const uint64_t max_cols = (TERM_SIZE + SPACE_GAP) / (1 + SPACE_GAP);
    t_map map = {.cols = 1,
                 .rows = req->entries->len,
                 .max = req->entries->len < max_cols ? req->entries->len
                                                     : max_cols};

    bool quoted = context_needs_padding_(req->quote_padding_context);
    uint64_t col_widths[(TERM_SIZE + SPACE_GAP) / (1 + SPACE_GAP)];
    calc_cols_(req->entries, &map, &quoted, col_widths);

    if (!put_dir_header_(req->buffer, req->dir_header)) {
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

            if (!escaped_out(out, entry->name, scan.quote, quoted)) {
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

static void update_list_stats_(t_list_stats *stats, const t_entry *entry) {
    if (entry->info->links->len > stats->max_len_links) {
        stats->max_len_links = entry->info->links->len;
    }

    if (entry->info->size->len > stats->max_len_sizes) {
        stats->max_len_sizes = entry->info->size->len;
    }

    if (entry->info->perm->len > stats->max_len_perm) {
        stats->max_len_perm = entry->info->perm->len;
    }

    if (!stats->have_quote) {
        t_shell_scan scan;
        shell_scan_str(entry->name, &scan);
        stats->have_quote = scan.quote != '\0';
    }

    stats->total += entry->info->blocks;
}

static void collect_list_stats_(const t_array *entries, t_list_stats *stats) {
    *stats = (t_list_stats){0};

    for (uint64_t index = 0; index < entries->len; ++index) {
        const t_entry *entry = entries->data[index];
        update_list_stats_(stats, entry);
    }
}

static bool print_list_(const t_print_request *req) {
    t_list_stats sizes;
    collect_list_stats_(req->entries, &sizes);
    apply_list_width_context_(req->list_width_context, &sizes);

    sizes.have_quote =
        sizes.have_quote || context_needs_padding_(req->quote_padding_context);

    if (!put_dir_header_(req->buffer, req->dir_header)) {
        return false;
    }

    if (req->print_total) {
        if (!put_mem(req->buffer, "total ", 6) ||
            !put_uint_(req->buffer, (sizes.total + 1) / 2) ||
            !put_mem(req->buffer, "\n", 1)) {
            return false;
        }
    }

    return print_list_rows_(req->buffer, req->entries, &sizes);
}

static bool left_pad_(t_str *out, const uint64_t src_len,
                      const uint64_t max_size) {
    uint64_t count = max_size - src_len;

    while (count) {
        if (out->len == out->cap - 1 && !flush_str(out)) {
            return false;
        }

        const uint64_t avail = (out->cap - 1) - out->len;
        const uint64_t to_fill = count < avail ? count : avail;
        ft_memset(out->str + out->len, ' ', (size_t)to_fill);
        out->len += to_fill;
        out->str[out->len] = '\0';
        count -= to_fill;
    }

    return true;
}

static bool put_uint_(t_str *out, uint64_t value) {
    char digits[32];
    size_t index = sizeof(digits);

    do {
        digits[--index] = (char)('0' + (value % 10));
        value /= 10;
    } while (value > 0);

    return put_mem(out, digits + index, (uint64_t)(sizeof(digits) - index));
}

static bool print_list_rows_(t_str *out, const t_array *array,
                             const t_list_stats *sizes) {
    for (uint64_t index = 0; index < array->len; ++index) {
        const t_entry *entry = array->data[index];
        t_shell_scan scan;

        shell_scan_str(entry->name, &scan);

        if (!put_mem(out, entry->info->perm->str, entry->info->perm->len) ||
            !left_pad_(out, entry->info->perm->len, sizes->max_len_perm) ||
            !put_mem(out, " ", 1) ||
            !left_pad_(out, entry->info->links->len, sizes->max_len_links) ||
            !put_mem(out, entry->info->links->str, entry->info->links->len) ||
            !put_mem(out, " ", 1) ||
            !put_mem(out, entry->info->username->str,
                     entry->info->username->len) ||
            !put_mem(out, " ", 1) ||
            !put_mem(out, entry->info->groupname->str,
                     entry->info->groupname->len) ||
            !put_mem(out, " ", 1) ||
            !left_pad_(out, entry->info->size->len, sizes->max_len_sizes) ||
            !put_mem(out, entry->info->size->str, entry->info->size->len) ||
            !put_mem(out, " ", 1) ||
            !put_mem(out, entry->info->dt->str, entry->info->dt->len) ||
            !put_mem(out, " ", 1) ||
            !escaped_out(out, entry->name, scan.quote, sizes->have_quote)) {
            return false;
        }

        if (entry->info->symlink && entry->info->symlink->len > 0) {
            if (!put_mem(out, " -> ", 4) ||
                !put_mem(out, entry->info->symlink->str,
                         entry->info->symlink->len)) {
                return false;
            }
        }

        if (!put_mem(out, "\n", 1)) {
            return false;
        }
    }

    return true;
}
