#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#include "../include/ft_arena.h"
#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_entry.h"
#include "../include/ft_helper.h"
#include "../include/ft_print_list.h"
#include "../include/ft_str.h"

typedef struct {
    uint64_t total;
    uint64_t buf_size;
    uint64_t max_len_links;
    uint64_t max_len_sizes;
    uint64_t max_len_perm;
    bool have_quote;
} t_sizes;

typedef struct {
    t_str *space;
    t_str *quote;
    t_str *new_line;
    t_str *dubble_colon;
} t_spacing;

#ifndef HEADER_PREFIX_LEN
#define HEADER_PREFIX_LEN UINT64_C(6)
#endif /* ifndef HEADER_PREFIX_LEN */

static void get_sizes_(t_array *array, t_sizes *sizes);
static void printer_(Arena *arena, t_array *array, t_str *buf,
                     const t_spacing *spacing, const t_sizes *sizes);
static void left_pad_(Arena *arena, t_str *buffer, uint64_t src_len,
                      uint64_t max_size);
static bool have_quotes_(t_array *array);

// todo: err msg when someting goes wrong
void print_list(t_array *array, t_entry *dir_entry, bool print_total,
                uint64_t min_len_links, uint64_t min_len_sizes,
                bool force_quote_padding) {
    ASSERT_NOTNULL(array);

    Arena *arena = ArenaAlloc(ARENA_SIZE);
    if (!arena) {
        return;
    }
    ArenaSetAutoAlign(arena, 8);

    t_sizes sizes = {.have_quote = force_quote_padding || have_quotes_(array),
                     .max_len_links = min_len_links,
                     .max_len_sizes = min_len_sizes};
    get_sizes_(array, &sizes);

    char space_buf[] = " ";
    char quote_buf[] = "'";
    char new_line_buf[] = "\n";
    char dubble_colon_buf[] = ":";
    char total_buf[] = "total ";

    t_str space = {.str = space_buf, .cap = 2, .len = 1, .pos = 0};
    t_str quote = {.str = quote_buf, .cap = 2, .len = 1, .pos = 0};
    t_str new_line = {.str = new_line_buf, .cap = 2, .len = 1, .pos = 0};
    t_str dubble_colon = {
        .str = dubble_colon_buf, .cap = 2, .len = 1, .pos = 0};
    t_str total_str = {.str = total_buf, .cap = 7, .len = 6, .pos = 0};

    t_spacing spacing = {.space = &space,
                         .quote = &quote,
                         .new_line = &new_line,
                         .dubble_colon = &dubble_colon};

    if (dir_entry) {
        dir_entry = escape_entry(arena, dir_entry);
        if (!dir_entry) {
            goto done;
        }

        if (dir_entry->quoted && dir_entry->quoted->len) {
            sizes.buf_size += 2;
        }
        sizes.buf_size += dir_entry->name->len + 3; // 1 for :, 1 for \n
    }

    t_str *buf = init_str(arena, sizes.buf_size);
    if (!buf) {
        goto done;
    }

    if (dir_entry) {
        if (dir_entry->quoted && dir_entry->quoted->len) {
            cat_str(buf, dir_entry->quoted);
            cat_str(buf, dir_entry->name);
            cat_str(buf, dir_entry->quoted);
        } else {
            cat_str(buf, dir_entry->name);
        }

        cat_str(buf, spacing.dubble_colon);
        cat_str(buf, spacing.new_line);
    }

    if (print_total) {
        const t_str *total = uint_to_str(arena, (sizes.total + 1) / 2);
        if (!total) {
            goto done;
        }
        cat_str(buf, &total_str);
        cat_str(buf, total);
        cat_str(buf, &new_line);
    }

    printer_(arena, array, buf, &spacing, &sizes);
done:
    ArenaRelease(arena);
}

static void printer_(Arena *arena, t_array *array, t_str *buf,
                     const t_spacing *spacing, const t_sizes *sizes) {
    for (uint64_t index = 0; index < array->len; ++index) {
        const t_entry *entry = array->data[index];

        cat_str(buf, entry->info->perm);
        left_pad_(arena, buf, entry->info->perm->len, sizes->max_len_perm);
        cat_str(buf, spacing->space);

        left_pad_(arena, buf, entry->info->links->len, sizes->max_len_links);
        cat_str(buf, entry->info->links);
        cat_str(buf, spacing->space);

        cat_str(buf, entry->info->username);
        cat_str(buf, spacing->space);

        cat_str(buf, entry->info->groupname);
        cat_str(buf, spacing->space);

        left_pad_(arena, buf, entry->info->size->len, sizes->max_len_sizes);
        cat_str(buf, entry->info->size);
        cat_str(buf, spacing->space);

        cat_str(buf, entry->info->dt);
        cat_str(buf, spacing->space);

        if (entry->quoted->len) {
            cat_str(buf, entry->quoted);
            cat_str(buf, entry->name);
            cat_str(buf, entry->quoted);
        } else {
            if (sizes->have_quote) {
                cat_str(buf, spacing->space);
            }
            cat_str(buf, entry->name);
        }

        if (entry->info->symlink) {
            append_chars_str(arena, buf, " -> ");
            cat_str(buf, entry->info->symlink);
        }

        cat_str(buf, spacing->new_line);
    }

    write(STDOUT_FILENO, buf->str, buf->len);
}

static void get_sizes_(t_array *array, t_sizes *sizes) {
    for (uint64_t i = 0; i < array->len; ++i) {
        t_entry *e = array->data[i];

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

    uint64_t total_str_len = len_of_nbr((sizes->total + 1) / 2);
    sizes->buf_size = 6 + total_str_len + 1; // "total " + number + '\n'

    for (uint64_t i = 0; i < array->len; ++i) {
        t_entry *e = array->data[i];
        uint64_t row = 0;

        row += e->info->perm->len + 1;
        row += (sizes->max_len_perm - e->info->perm->len); // left_pad

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

        e = escape_entry(array->arena, e);
        // array->data[i] = e;
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
        const t_entry *entry = array->data[index];
        if (entry->quoted->len) {
            return true;
        }
    }

    return false;
}
