#include <dirent.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "./ft_arena.h"
#include "./ft_array.h"
#include "./ft_shell_escape.h"
#include "./ft_str.h"
#include "./ft_utils.h"
#include "./ft_walk_entry.h"

typedef struct {
    t_entry entry;
    t_str name;
    t_str path;
    char buf[];
} t_scratch_dir_entry;

t_entry *entry_new_path(const t_str *path, const struct stat *st,
                        const bool is_operand) {
    t_entry *entry = malloc(sizeof(*entry));
    if (!entry) {
        return NULL;
    }

    *entry = (t_entry){0};
    entry->path = str_dup(path);
    if (!entry->path) {
        free(entry);
        return NULL;
    }

    entry->name = entry->path;
    entry->st = *st;
    entry->is_operand = is_operand;
    shell_scan_str(entry->path, &entry->name_scan);
    return entry;
}

t_entry *entry_new_dirent(t_arena *scratch, const struct dirent *dp,
                          const t_str *dir_path) {
    const size_t name_len = (size_t)ft_strlen(dp->d_name);
    const bool need_slash =
        dir_path->len != 0 && dir_path->str[dir_path->len - 1] != '/';
    const size_t prefix_len = dir_path->len + (need_slash ? 1 : 0);

    t_scratch_dir_entry *ent =
        arena_push(scratch, sizeof(*ent) + prefix_len + name_len + 1);
    if (!ent) {
        return NULL;
    }

    ft_memcpy(ent->buf, dir_path->str, dir_path->len);
    if (need_slash) {
        ent->buf[dir_path->len] = '/';
    }
    ft_memcpy(ent->buf + prefix_len, dp->d_name, name_len + 1);

    ent->path = (t_str){.str = ent->buf, .len = prefix_len + name_len};
    ent->name = (t_str){.str = ent->buf + prefix_len, .len = name_len};
    ent->entry = (t_entry){.name = &ent->name, .path = &ent->path};
    shell_scan_str(&ent->name, &ent->entry.name_scan);

    return &ent->entry;
}

void entry_free(t_entry *entry) {
    if (!entry) {
        return;
    }

    if (entry->path && entry->path != entry->name) {
        str_free(entry->path);
    }

    if (entry->name) {
        str_free(entry->name);
    }

    free(entry);
}

void entries_free(t_array *entries) {
    for (size_t index = 0; index < entries->len; ++index) {
        entry_free(entries->data[index]);
    }

    array_clear(entries);
}
