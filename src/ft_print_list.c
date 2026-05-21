#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#include "../include/ft_entry.h"
#include "../include/ft_printer.h"
#include "../include/ft_printer_helper.h"
#include "../include/ft_shell_escape.h"
#include "../include/ft_str.h"

#include "../libft/include/libft.h"

typedef struct {
    uint64_t total;
    uint64_t max_len_links;
    uint64_t max_len_sizes;
    uint64_t max_len_perm;
    bool have_quote;
} t_sizes;

static bool printer_(t_str *out, t_array *array, const t_sizes *sizes);
static void get_sizes_(t_array *array, t_sizes *sizes);
static bool left_pad_(t_str *out, uint64_t src_len, uint64_t max_size);
static bool put_uint_(t_str *out, uint64_t value);

void print_list(t_ps *ps) {
    t_sizes sizes = {.have_quote = ps->quote_padding || have_quotes(ps->array),
                     .max_len_links = ps->min_len_links,
                     .max_len_sizes = ps->min_len_sizes};
    char buffer[OUTPUT_BUFFER_CAP];
    t_str out = {.str = buffer, .cap = sizeof(buffer), .len = 0, .pos = 0};
    out.str[0] = '\0';

    get_sizes_(ps->array, &sizes);
    if (!put_dir_header(&out, ps->dir_entry)) {
        goto done;
    }

    if (ps->print_total) {
        if (!put_mem(&out, "total ", 6) ||
            !put_uint_(&out, (sizes.total + 1) / 2) ||
            !put_mem(&out, "\n", 1)) {
            goto done;
        }
    }

    if (!printer_(&out, ps->array, &sizes)) {
        goto done;
    }

    (void)flush_str(&out);
done:
    return;
}

static bool printer_(t_str *out, t_array *array, const t_sizes *sizes) {
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

static void get_sizes_(t_array *array, t_sizes *sizes) {
    for (uint64_t i = 0; i < array->len; ++i) {
        const t_entry *e = array->data[i];

        if (e->info->links->len > sizes->max_len_links) {
            sizes->max_len_links = e->info->links->len;
        }

        if (e->info->size->len > sizes->max_len_sizes) {
            sizes->max_len_sizes = e->info->size->len;
        }

        if (e->info->perm->len > sizes->max_len_perm) {
            sizes->max_len_perm = e->info->perm->len;
        }

        sizes->total += e->info->blocks;
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
