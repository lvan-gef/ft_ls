#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#include "../include/ft_arena.h"
#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_path.h"
#include "../include/ft_print_list.h"
#include "../include/ft_str.h"

typedef struct {
    uint64_t total;
    uint64_t buf_size;
    uint64_t max_len_links;
    uint64_t max_len_sizes;
    bool have_quote;
} t_sizes;

#ifndef HEADER_PREFIX_LEN
#define HEADER_PREFIX_LEN UINT64_C(6)
#endif /* ifndef HEADER_PREFIX_LEN */

static void get_sizes_(t_array *array, t_sizes *sizes);
static uint64_t len_of_nbr_(uint64_t nbr);
static void left_pad_(Arena *arena, t_str *buffer, uint64_t src_len,
                      uint64_t max_size);
static bool have_quotes_(t_array *array);

void print_list(t_array *array, t_str *path, bool print_total) {
    ASSERT_NOTNULL(array);

    Arena *arena = ArenaAlloc(ARENA_SIZE);
    if (!arena) {
        return;
    }
    ArenaSetAutoAlign(arena, 8);

    t_sizes sizes = {.have_quote = have_quotes_(array)};
    get_sizes_(array, &sizes);

    if (path) {
        sizes.buf_size += (path->len + 1 + 1); // 1 for :, 1 for \n
    }

    t_str *buf = init_str(arena, sizes.buf_size);
    if (!buf) {
        goto done;
    }

    if (path) {
        cat_str(buf, path);
        append_chars_str(arena, buf, ":");
        append_chars_str(arena, buf, "\n");
    }

    if (print_total) {
        t_str *total = uint_to_str(arena, (sizes.total + 1) / 2);
        if (!total) {
            goto done;
        }
        append_chars_str(arena, buf, "total ");
        cat_str(buf, total);
        append_chars_str(arena, buf, "\n");
    }

    for (uint64_t index = 0; index < array->len; ++index) {
        t_entry *entry = array->data[index];

        cat_l_str(buf, entry->info->perm, sizes.buf_size);
        append_chars_str(arena, buf, " ");

        left_pad_(arena, buf, entry->info->links->len, sizes.max_len_links);
        cat_l_str(buf, entry->info->links, sizes.buf_size);
        append_chars_str(arena, buf, " ");

        cat_l_str(buf, entry->info->username, sizes.buf_size);
        append_chars_str(arena, buf, " ");

        cat_l_str(buf, entry->info->groupname, sizes.buf_size);
        append_chars_str(arena, buf, " ");

        left_pad_(arena, buf, entry->info->size->len, sizes.max_len_sizes);
        cat_l_str(buf, entry->info->size, sizes.buf_size);
        append_chars_str(arena, buf, " ");

        cat_l_str(buf, entry->info->dt, sizes.buf_size);
        append_chars_str(arena, buf, " ");

        if (entry->quoted->len) {
            append_chars_str(arena, buf, "'");
            cat_l_str(buf, entry->name, sizes.buf_size);
            append_chars_str(arena, buf, "'");
        } else {
            if (sizes.have_quote) {
                append_chars_str(arena, buf, " ");
            }
            cat_l_str(buf, entry->name, sizes.buf_size);
        }

        if (entry->info->symlink) {
            append_chars_str(arena, buf, " -> ");
            cat_l_str(buf, entry->info->symlink, sizes.buf_size);
        }

        append_chars_str(arena, buf, "\n");
    }

    write(STDOUT_FILENO, buf->str, buf->len);
done:
    ArenaRelease(arena);
}

static void get_sizes_(t_array *array, t_sizes *sizes) {
    for (uint64_t i = 0; i < array->len; ++i) {
        t_entry *e = array->data[i];

        if (e->info->links->len > sizes->max_len_links)
            sizes->max_len_links = e->info->links->len;

        if (e->info->size->len > sizes->max_len_sizes)
            sizes->max_len_sizes = e->info->size->len;

        sizes->total += e->info->blocks;
    }

    uint64_t total_str_len = len_of_nbr_((sizes->total + 1) / 2);
    sizes->buf_size = 6 + total_str_len + 1; // "total " + number + '\n'

    for (uint64_t i = 0; i < array->len; ++i) {
        t_entry *e = array->data[i];
        uint64_t row = 0;

        row += e->info->perm->len + 1;

        row += (sizes->max_len_links - e->info->links->len); // left_pad
        row += e->info->links->len + 1;

        row += e->info->username->len + 1;
        row += e->info->groupname->len + 1;

        row += (sizes->max_len_sizes - e->info->size->len); // left_pad
        row += e->info->size->len + 1;

        row += e->info->dt->len + 1;

        if (e->quoted->len) {
            row += 2;
        } else if (sizes->have_quote) {
            row += 1;
        }

        row += e->name->len;

        if (e->info->symlink) {
            row += 4; // " -> "
            row += e->info->symlink->len;
        }

        row += 1; // '\n'
        sizes->buf_size += row;
    }

    if (!sizes->max_len_sizes) {
        sizes->buf_size += 2;
    }
}

static uint64_t len_of_nbr_(uint64_t nbr) {
    uint64_t len = 1;

    while (nbr >= 10) {
        nbr /= 10;
        ++len;
    }

    return len;
}

static void left_pad_(Arena *arena, t_str *buffer, uint64_t src_len,
                      uint64_t max_size) {
    ASSERT_LE(src_len, max_size);
    uint64_t differ = max_size - src_len;

    for (uint64_t index = 0; index < differ; ++index) {
        append_chars_str(arena, buffer, " ");
    }
}

static bool have_quotes_(t_array *array) {
    for (uint64_t index = 0; index < array->len; ++index) {
        t_entry *entry = array->data[index];
        if (entry->quoted->len) {
            return true;
        }
    }

    return false;
}
