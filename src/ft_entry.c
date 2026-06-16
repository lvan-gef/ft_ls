#include <errno.h>
#include <grp.h>
#include <linux/limits.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "../include/ft_arena.h"
#include "../include/ft_entry.h"
#include "../include/ft_free_list.h"
#include "../include/ft_str.h"

#include "../libft/include/libft.h"

static t_str *arena_join_dir_path_(Arena *arena, const t_str *lhs,
                                   const t_str *rhs);
static t_str *fl_join_dir_path_(free_list *fl, const t_str *lhs,
                                const t_str *rhs);
static void fill_join_dir(t_str *path, const t_str *lhs, const t_str *rhs,
                          bool need_slash);

t_entry *arena_new_entry(Arena *arena, const t_entry *parent,
                         const struct dirent *dp) {
    Arena_Mark mark = arena_get_mark(arena);
    t_entry *ent = arena_push(arena, sizeof(*ent));
    if (!ent) {
        return NULL;
    }

    *ent = (t_entry){0};
    const size_t name_len = ft_strlen(dp->d_name);
    ent->name = arena_init_str(arena, name_len);
    if (!ent->name) {
        goto failed;
    }

    ft_memcpy(ent->name->str, dp->d_name, name_len);
    ent->name->len = name_len;
    ent->name->str[ent->name->len] = '\0';
    ent->path = NULL;
    ent->parent_path = parent->path;
    ent->stat_unavailable = false;
    ent->is_operand = false;
    ent->st = (struct stat){0};
    return ent;
failed:
    arena_pop_to_mark(arena, mark);
    return NULL;
}

bool arena_entry_path(Arena *arena, t_entry *entry) {
    if (entry->path) {
        return true;
    }

    if (!entry->parent_path || !entry->name) {
        return false;
    }

    entry->path = arena_join_dir_path_(arena, entry->parent_path, entry->name);
    return entry->path != NULL;
}

t_entry *fl_dup_entry(free_list *fl, const t_entry *src,
                      const bool is_operand) {
    t_entry *entry = fl_alloc(fl, sizeof(*entry));
    if (!entry) {
        return NULL;
    }

    *entry = (t_entry){0};
    if (!src->path) {
        if (!src->parent_path || !src->name) {
            goto failed;
        }

        entry->path = fl_join_dir_path_(fl, src->parent_path, src->name);
    } else {
        entry->path = fl_dup_str(fl, src->path);
    }

    if (!entry->path) {
        goto failed;
    }

    entry->st = src->st;
    entry->stat_unavailable = src->stat_unavailable;
    entry->is_operand = is_operand;
    return entry;
failed:
    fl_free(fl, entry);
    return NULL;
}

t_str *arena_read_symlink(Arena *arena, const t_str *path,
                          const uint64_t target_size, int *read_err) {
    if (read_err) {
        *read_err = 0;
    }

    uint64_t cap = (target_size > 0) ? target_size + 1 : (uint64_t)PATH_MAX;
    while (true) {
        Arena_Mark mark = arena_get_mark(arena);
        t_str *new_str = arena_init_str(arena, cap);
        if (!new_str) {
            return NULL;
        }

        const size_t read_size = (cap > (size_t)-1) ? (size_t)-1 : (size_t)cap;
        const ssize_t len = readlink(path->str, new_str->str, read_size);
        if (len < 0) {
            const int err = errno;
            arena_pop_to_mark(arena, mark);

            if (err == ENOENT || err == EINVAL || err == EACCES ||
                err == EPERM) {
                if (read_err) {
                    *read_err = err;
                }
                return arena_init_str(arena, 1);
            }

            return NULL;
        }

        if ((uint64_t)len < cap) {
            new_str->len = (uint64_t)len;
            new_str->str[new_str->len] = '\0';
            return new_str;
        }

        arena_pop_to_mark(arena, mark);
        if (cap > 0x3FFFFFFFFFFFFFFF) {
            return NULL;
        }

        cap *= 2;
    }
}

void free_entry(free_list *fl, const t_entry *entry) {
    if (!entry) {
        return;
    }

    if (entry->name) {
        fl_free_str(fl, entry->name);
    }

    if (entry->path && entry->path != entry->name) {
        fl_free_str(fl, entry->path);
    }

    fl_free(fl, entry);
}

static t_str *arena_join_dir_path_(Arena *arena, const t_str *lhs,
                                   const t_str *rhs) {
    const bool need_slash = lhs->len == 0 || lhs->str[lhs->len - 1] != '/';
    const uint64_t total_len = lhs->len + rhs->len + (need_slash ? 1U : 0U);
    t_str *path = arena_init_str(arena, total_len);

    if (!path) {
        return NULL;
    }

    fill_join_dir(path, lhs, rhs, need_slash);
    return path;
}

static t_str *fl_join_dir_path_(free_list *fl, const t_str *lhs,
                                const t_str *rhs) {
    const bool need_slash = lhs->len == 0 || lhs->str[lhs->len - 1] != '/';
    const uint64_t total_len = lhs->len + rhs->len + (need_slash ? 1U : 0U);
    t_str *path = fl_init_str(fl, total_len);

    if (!path) {
        return NULL;
    }

    fill_join_dir(path, lhs, rhs, need_slash);
    return path;
}

static void fill_join_dir(t_str *path, const t_str *lhs, const t_str *rhs,
                          bool need_slash) {
    ft_memcpy(path->str, lhs->str + lhs->pos, (size_t)lhs->len);
    path->len = lhs->len;
    if (need_slash) {
        path->str[path->len++] = '/';
    }

    ft_memcpy(path->str + path->len, rhs->str + rhs->pos, (size_t)rhs->len);
    path->len += rhs->len;
    path->str[path->len] = '\0';
}
