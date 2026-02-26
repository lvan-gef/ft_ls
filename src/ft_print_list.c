#include <stdint.h>
#include <unistd.h>

#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_print_list.h"
#include "ft_arena.h"
#include "ft_path.h"
#include "ft_str.h"

static uint64_t get_buf_size_(t_array *array);

// todo padding
// todo header
void print_list(t_array *array) {
    ASSERT_NOTNULL(array);

    Arena *arena = ArenaAlloc(ARENA_SIZE);
    if (!arena) {
        return;
    }
    ArenaSetAutoAlign(arena, 8);

    uint64_t buf_size = get_buf_size_(array);
    t_str *buffer = init_str(arena, (buf_size * array->len) + array->len);
    if (!buffer) {
        goto done;
    }

    t_str *space = create_str(arena, " ");
    if (!space) {
        goto done;
    }

    t_str *arrow = create_str(arena, " -> ");
    if (!arrow) {
        goto done;
    }

    t_str *single_quote = create_str(arena, "'");
    if (!single_quote) {
        goto done;
    }

    t_str *newline = create_str(arena, "\n");
    if (!newline) {
        goto done;
    }

    for (uint64_t index = 0; index < array->len; ++index) {
        t_entry *entry = array->data[index];

        cat_l_str(buffer, entry->info->perm, buf_size);
        cat_l_str(buffer, space, buf_size);

        cat_l_str(buffer, entry->info->links, buf_size);
        cat_l_str(buffer, space, buf_size);

        cat_l_str(buffer, entry->info->username, buf_size);
        cat_l_str(buffer, space, buf_size);

        cat_l_str(buffer, entry->info->groupname, buf_size);
        cat_l_str(buffer, space, buf_size);

        cat_l_str(buffer, entry->info->size, buf_size);
        cat_l_str(buffer, space, buf_size);

        cat_l_str(buffer, entry->info->dt, buf_size);
        cat_l_str(buffer, space, buf_size);

        if (entry->quoted->len) {
            cat_l_str(buffer, single_quote, buf_size);
            cat_l_str(buffer, entry->name, buf_size);
            cat_l_str(buffer, single_quote, buf_size);
        } else {
            cat_l_str(buffer, entry->name, buf_size);
        }

        if (entry->info->symlink) {
            cat_l_str(buffer, arrow, buf_size);
            cat_l_str(buffer, entry->info->symlink, buf_size);
        }

        cat_l_str(buffer, newline, buf_size);
    }

    write(STDOUT_FILENO, buffer->str, buffer->len);
done:
    ArenaRelease(arena);
}

// todo: check of er een file is met quetes want dan 2 spaties voor
// elke filename die geen quetes heeft
static uint64_t get_buf_size_(t_array *array) {
    ASSERT_NOTNULL(array);

    uint64_t max = 0;
    for (uint64_t index = 0; index < array->len; ++index) {
        uint64_t total = 0;
        t_entry *entry = array->data[index];

        total += entry->info->perm->len + 1;
        total += entry->info->links->len + 1;
        total += entry->info->username->len + 1;
        total += entry->info->groupname->len + 1;
        total += entry->info->size->len + 1;
        total += entry->info->dt->len + 1;

        if (entry->quoted->len) {
            total += 2;
        }
        total += entry->name->len;

        if (entry->info->symlink) {
            total += (1 + 2 + 1); // 1 space, 2 for ->, 1 space
            total += entry->info->symlink->len;
        }

        if (total > max) {
            max = total;
        }
    }

    return max;
}
