#include <dirent.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "../include/ft_str.h"

#include "../../libft/include/libft.h"

#include "./ft_arena.h"
#include "./ft_shell_escape.h"
#include "./ft_walk_entry.h"

typedef struct {
    t_entry entry;
    t_str name;
    char name_buf[];
} t_scratch_dir_entry;

static t_str *join_dir_path_(Arena *scratch, const t_str *lhs,
                             const t_str *rhs);

t_entry *entry_new_file_operand(const t_str *path, const struct stat *st) {
    t_entry *entry = malloc(sizeof(*entry));
    if (!entry) {
        return NULL;
    }

    *entry = (t_entry){0};
    entry->name = str_dup(path);
    if (!entry->name) {
        free(entry);
        return NULL;
    }

    entry->path = entry->name;
    entry->st = *st;
    entry->is_operand = true;
    shell_scan_str(entry->name, &entry->name_scan);

    return entry;
}

t_entry *entry_new_path(const t_str *path, const struct stat *st,
                        const bool is_operand) {
    t_entry *entry = malloc(sizeof(*entry));
    if (!entry) {
        return NULL;
    }

    *entry = (t_entry){0};
    entry->path = str_dup(path);
    entry->st = *st;
    entry->is_operand = is_operand;

    if (!entry->path) {
        entry_free(entry);
        return NULL;
    }

    shell_scan_str(entry->path, &entry->name_scan);

    return entry;
}

t_entry *entry_new_dirent(Arena *scratch, const struct dirent *dp) {
    const uint64_t name_len = (uint64_t)ft_strlen(dp->d_name);
    if (name_len > UINT64_MAX - (uint64_t)sizeof(t_scratch_dir_entry) - 1) {
        return NULL;
    }

    t_scratch_dir_entry *ent = arena_push(scratch, sizeof(*ent) + name_len + 1);
    if (!ent) {
        return NULL;
    }

    str_init(&ent->name, ent->name_buf, name_len);
    str_copy_cstr(&ent->name, dp->d_name, name_len);
    ent->entry = (t_entry){.name = &ent->name,
                           .path = NULL,
                           .st = (struct stat){0},
                           .stat_unavailable = false,
                           .is_operand = false};
    shell_scan_str(ent->entry.name, &ent->entry.name_scan);
    return &ent->entry;
}

bool entry_build_path(Arena *scratch, t_entry *entry,
                      const t_str *parent_path) {
    if (entry->path) {
        return true;
    }
    if (!parent_path || !entry->name) {
        return false;
    }

    entry->path = join_dir_path_(scratch, parent_path, entry->name);
    return entry->path != NULL;
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

void entry_del(void *ptr) {
    entry_free((t_entry *)ptr);
}

static t_str *join_dir_path_(Arena *scratch, const t_str *lhs,
                             const t_str *rhs) {
    const bool need_slash = lhs->len != 0 && lhs->str[lhs->len - 1] != '/';
    const uint64_t slash_len = need_slash ? 1U : 0U;
    if (lhs->len > UINT64_MAX - rhs->len - slash_len) {
        return NULL;
    }

    const uint64_t total_len = lhs->len + rhs->len + slash_len;
    if (total_len > UINT64_MAX - (uint64_t)sizeof(t_str) - 1) {
        return NULL;
    }

    t_str *path = arena_push(scratch, sizeof(*path) + total_len + 1);
    if (!path) {
        return NULL;
    }

    str_init(path, (char *)(path + 1), total_len);

    ft_memcpy(path->str, lhs->str, (size_t)lhs->len);
    path->len = lhs->len;
    if (need_slash) {
        path->str[path->len++] = '/';
    }

    ft_memcpy(path->str + path->len, rhs->str, (size_t)rhs->len);
    path->len += rhs->len;
    path->str[path->len] = '\0';

    return path;
}
