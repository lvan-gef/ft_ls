#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#include "../include/ft_entry.h"
#include "../include/ft_printer.h"
#include "../include/ft_printer_helper.h"
#include "../include/ft_shell_escape.h"
#include "../include/ft_str.h"

#include "../libft/include/libft.h"

static void printer_(t_str *out, t_array *array, const t_list_stats *sizes);
static bool left_pad_(t_str *out, uint64_t src_len, uint64_t max_size);
static bool put_uint_(t_str *out, uint64_t value);

void print_list(t_ps *ps) {
    t_list_stats sizes = ps->stats;

    if (sizes.max_len_links < ps->min_len_links) {
        sizes.max_len_links = ps->min_len_links;
    }

    if (sizes.max_len_sizes < ps->min_len_sizes) {
        sizes.max_len_sizes = ps->min_len_sizes;
    }

    sizes.have_quote = sizes.have_quote || ps->quote_padding;
    if (!put_dir_header(ps->buffer, ps->dir_entry)) {
        return;
    }

    if (ps->print_total) {
        if (!put_mem(ps->buffer, "total ", 6) ||
            !put_uint_(ps->buffer, (sizes.total + 1) / 2) ||
            !put_mem(ps->buffer, "\n", 1)) {
            return;
        }
    }

    printer_(ps->buffer, ps->array, &sizes);
}

static void printer_(t_str *out, t_array *array, const t_list_stats *sizes) {
    for (uint64_t index = 0; index < array->len; ++index) {
        const t_entry *entry = array->data[index];

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
            !escaped_out(out, entry->name, entry->quote, sizes->have_quote)) {
            return;
        }

        if (entry->info->symlink && entry->info->symlink->len > 0) {
            if (!put_mem(out, " -> ", 4) ||
                !put_mem(out, entry->info->symlink->str,
                         entry->info->symlink->len)) {
                return;
            }
        }

        if (!put_mem(out, "\n", 1)) {
            return;
        }
    }
}

static bool left_pad_(t_str *out, uint64_t src_len, uint64_t max_size) {
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
