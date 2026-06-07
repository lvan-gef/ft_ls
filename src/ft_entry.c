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
#include "../include/ft_shell_escape.h"
#include "../include/ft_str.h"
#include "../include/ft_helper.h"

#include "../libft/include/libft.h"

#ifndef CACHE_SIZE
#define CACHE_SIZE UINT64_C(8)
#endif

#ifndef PERMISSION_SIZE
#define PERMISSION_SIZE UINT64_C(12)
#endif // ifndef PERMISSION_SIZE //

#ifndef DT_LEN
#define DT_LEN UINT64_C(13)
#endif // ifndef DT_LEN //

typedef struct {
    char name[LOGIN_NAME_MAX];
    uint64_t id;
} t_id_cache_entry;

static uint64_t user_index = 0;
static t_id_cache_entry user_cache[CACHE_SIZE] = {0};

static uint64_t group_index = 0;
static t_id_cache_entry group_cache[CACHE_SIZE] = {0};

static bool fill_file_info_(const t_alloc *alloc, t_file_info *info,
                            const t_entry *entry);
static t_str *join_dir_path_(const t_alloc *alloc, const t_str *lhs,
                             const t_str *rhs);
static void fill_dir_entry_display_(t_entry *entry, const t_entry *src);
static t_str *get_perm_(const t_alloc *alloc, const t_entry *entry);
static t_str *get_user_(const t_alloc *alloc, uid_t user_id);
static t_str *get_group_(const t_alloc *alloc, gid_t group_id);
static t_str *get_dt_(const t_alloc *alloc, const struct timespec *ctim);
static t_str *get_symlink_(const t_alloc *alloc, const t_entry *entry);
static void free_entry_cb_(free_list *fl, void *ptr);
static void free_info_(free_list *fl, t_file_info *info);
static void free_info_cb_(free_list *fl, void *ptr);
static void free_str_cb_(free_list *fl, void *ptr);
static char *cache_lookup_(t_id_cache_entry *cache, uint64_t id);
static void cache_store_(t_id_cache_entry *cache, uint64_t *next, uint64_t id,
                         const char *name);

t_entry *new_entry(const t_alloc *alloc, const t_entry *entry,
                   const struct dirent *dp) {
    Arena_Mark mark = {0};
    t_entry *ent = alloc_mem(alloc, &mark, sizeof(*ent));
    if (!ent) {
        return NULL;
    }

    t_shell_scan scan;
    shell_scan_cstr(dp->d_name, &scan);
    ent->name = init_str(alloc, scan.len);
    if (!ent->name) {
        goto failed;
    }

    ft_memcpy(ent->name->str, dp->d_name, scan.len);
    ent->name->len = scan.len;
    ent->name->str[ent->name->len] = '\0';
    ent->path = NULL;
    ent->symlink = NULL;
    ent->parent_path = entry->path;
    ent->quote = scan.quote;
    ent->path_has_colon = entry->path_has_colon ||
                          ft_memchr(dp->d_name, ':', (size_t)scan.len) != NULL;
    ent->is_operand = false;
    ent->symlink_ready = false;
    ent->info = NULL;
    ent->st = (struct stat){0};
    ent->display_len = scan.display_len;
    ent->padded_display_len = scan.padded_display_len;
    return ent;
failed:
    free_alloc(alloc, mark, ent, fl_free);
    return NULL;
}

bool get_file_info(const t_alloc *alloc, t_entry *entry) {
    Arena_Mark mark = {0};
    t_file_info *info = alloc_mem(alloc, &mark, sizeof(*info));
    if (!info) {
        goto failed;
    }

    *info = (t_file_info){0};
    if (!fill_file_info_(alloc, info, entry)) {
        goto failed;
    }

    entry->info = info;
    return true;

failed:
    free_alloc(alloc, mark, info, free_info_cb_);

    entry->info = NULL;
    return false;
}

void init_entry_display(t_entry *entry) {
    t_shell_scan scan;

    shell_scan_str(entry->name, &scan);
    entry->quote = scan.quote;
    entry->display_len = scan.display_len;
    entry->padded_display_len = scan.padded_display_len;
}

bool ensure_entry_path(const t_alloc *alloc, t_entry *entry) {
    if (entry->path) {
        return true;
    }

    if (!entry->parent_path || !entry->name) {
        return false;
    }

    entry->path = join_dir_path_(alloc, entry->parent_path, entry->name);
    return entry->path != NULL;
}

t_entry *dup_dir_entry(const t_alloc *alloc, const t_entry *src,
                       bool is_operand) {
    Arena_Mark mark = {0};
    t_entry *entry = alloc_mem(alloc, &mark, sizeof(*entry));
    if (!entry) {
        return NULL;
    }

    *entry = (t_entry){0};
    if (!src->path) {
        if (!src->parent_path || !src->name) {
            goto failed;
        }

        entry->path = join_dir_path_(alloc, src->parent_path, src->name);
    } else {
        entry->path = dup_str(alloc, src->path);
    }

    if (!entry->path) {
        goto failed;
    }

    entry->st = src->st;
    fill_dir_entry_display_(entry, src);
    entry->is_operand = is_operand;
    return entry;
failed:
    free_alloc(alloc, mark, entry, free_entry_cb_);
    return NULL;
}

t_str *read_symlink_target(const t_alloc *alloc, const t_str *path,
                           uint64_t target_size, int *read_err) {
    Arena_Mark mark = {0};

    if (read_err) {
        *read_err = 0;
    }

    if (alloc->kind == ALLOC_ARENA) {
        mark = arena_get_mark(alloc->as.arena);
    }

    uint64_t cap = (target_size > 0) ? target_size + 1 : (uint64_t)PATH_MAX;
    while (true) {
        t_str *new_str = init_str(alloc, cap);
        if (!new_str) {
            return NULL;
        }

        const size_t read_size = (cap > (size_t)-1) ? (size_t)-1 : (size_t)cap;
        ssize_t len = readlink(path->str, new_str->str, read_size);
        if (len < 0) {
            const int err = errno;
            free_alloc(alloc, mark, new_str, free_str_cb_);

            if (err == ENOENT || err == EINVAL || err == EACCES ||
                err == EPERM) {
                if (read_err) {
                    *read_err = err;
                }
                return init_str(alloc, 1);
            }

            return NULL;
        }

        if ((uint64_t)len < cap) {
            new_str->len = (uint64_t)len;
            new_str->str[new_str->len] = '\0';
            return new_str;
        }

        free_alloc(alloc, mark, new_str, free_str_cb_);

        if (cap > 0x3FFFFFFFFFFFFFFF) {
            return NULL;
        }

        cap *= 2;
    }
}

void free_entry(free_list *fl, t_entry *entry) {
    if (!entry) {
        return;
    }

    if (entry->name) {
        free_str(fl, entry->name);
    }

    if (entry->path && entry->path != entry->name) {
        free_str(fl, entry->path);
    }

    if (entry->symlink) {
        free_str(fl, entry->symlink);
    }

    if (entry->info) {
        free_info_(fl, entry->info);
    }

    fl_free(fl, entry);
}

static bool fill_file_info_(const t_alloc *alloc, t_file_info *info,
                            const t_entry *entry) {
    info->perm = get_perm_(alloc, entry);
    if (!info->perm) {
        return false;
    }

    info->links = uint_to_str(alloc, entry->st.st_nlink);
    if (!info->links) {
        return false;
    }

    info->username = get_user_(alloc, entry->st.st_uid);
    if (!info->username) {
        return false;
    }

    info->groupname = get_group_(alloc, entry->st.st_gid);
    if (!info->groupname) {
        return false;
    }

    info->size = uint_to_str(alloc, (uint64_t)entry->st.st_size);
    if (!info->size) {
        return false;
    }

    info->dt = get_dt_(alloc, &entry->st.st_mtim);
    if (!info->dt) {
        return false;
    }

    info->symlink = get_symlink_(alloc, entry);
    if (!info->symlink) {
        return false;
    }

    info->blocks = (uint64_t)entry->st.st_blocks;
    return true;
}

static t_str *join_dir_path_(const t_alloc *alloc, const t_str *lhs,
                             const t_str *rhs) {
    const bool need_slash = lhs->len == 0 || lhs->str[lhs->len - 1] != '/';
    const uint64_t total_len = lhs->len + rhs->len + (need_slash ? 1U : 0U);
    t_str *path = init_str(alloc, total_len);

    if (!path) {
        return NULL;
    }

    ft_memcpy(path->str, lhs->str + lhs->pos, (size_t)lhs->len);
    path->len = lhs->len;
    if (need_slash) {
        path->str[path->len++] = '/';
    }

    ft_memcpy(path->str + path->len, rhs->str + rhs->pos, (size_t)rhs->len);
    path->len += rhs->len;
    path->str[path->len] = '\0';
    return path;
}

static void fill_dir_entry_display_(t_entry *entry, const t_entry *src) {
    t_shell_scan scan;

    shell_scan_str(entry->path, &scan);
    entry->quote = scan.quote;
    entry->path_has_colon = src->path_has_colon;
    entry->display_len = scan.display_len;
    entry->padded_display_len = scan.padded_display_len;
    if (entry->quote == '\0' && entry->path_has_colon) {
        entry->quote = '\'';
        entry->display_len = entry->path->len + 2;
        entry->padded_display_len = entry->display_len;
    }
}

static t_str *get_perm_(const t_alloc *alloc, const t_entry *entry) {
    t_str *str = init_str(alloc, PERMISSION_SIZE);
    if (!str) {
        return NULL;
    }

    uint64_t index = 0;
    if ((entry->st.st_mode & S_IFMT) == S_IFLNK) {
        str->str[index++] = 'l';
    } else if ((entry->st.st_mode & S_IFMT) == S_IFREG) {
        str->str[index++] = '-';
    } else if ((entry->st.st_mode & S_IFMT) == S_IFDIR) {
        str->str[index++] = 'd';
    }

    str->str[index++] = entry->st.st_mode & S_IRUSR ? 'r' : '-';
    str->str[index++] = entry->st.st_mode & S_IWUSR ? 'w' : '-';
    str->str[index++] = entry->st.st_mode & S_IXUSR ? 'x' : '-';
    str->str[index++] = entry->st.st_mode & S_IRGRP ? 'r' : '-';
    str->str[index++] = entry->st.st_mode & S_IWGRP ? 'w' : '-';
    str->str[index++] = entry->st.st_mode & S_IXGRP ? 'x' : '-';
    str->str[index++] = entry->st.st_mode & S_IROTH ? 'r' : '-';
    str->str[index++] = entry->st.st_mode & S_IWOTH ? 'w' : '-';
    str->str[index++] = entry->st.st_mode & S_IXOTH ? 'x' : '-';

    str->len = index;
    str->str[index] = '\0';
    return str;
}

static t_str *get_user_(const t_alloc *alloc, uid_t user_id) {
    const char *name = cache_lookup_(user_cache, (uint64_t)user_id);
    if (name) {
        return create_str(alloc, name);
    }

    const struct passwd *pwd = getpwuid(user_id);
    t_str *new_str = NULL;
    if (pwd) {
        new_str = create_str(alloc, pwd->pw_name);
        if (!new_str) {
            return NULL;
        }
        cache_store_(user_cache, &user_index, (uint64_t)user_id, pwd->pw_name);
    } else {
        const int err = errno;

        new_str = uint_to_str(alloc, (uint64_t)user_id);
        if (!new_str) {
            return NULL;
        }

        if (!err) {
            cache_store_(user_cache, &user_index, (uint64_t)user_id,
                         new_str->str);
        }
    }

    return new_str;
}

static t_str *get_group_(const t_alloc *alloc, gid_t group_id) {
    const char *name = cache_lookup_(group_cache, (uint64_t)group_id);
    if (name) {
        return create_str(alloc, name);
    }

    const struct group *grp = getgrgid(group_id);
    t_str *new_str = NULL;
    if (grp) {
        new_str = create_str(alloc, grp->gr_name);
        if (!new_str) {
            return NULL;
        }

        cache_store_(group_cache, &group_index, (uint64_t)group_id,
                     grp->gr_name);
    } else {
        const int err = errno;

        new_str = uint_to_str(alloc, (uint64_t)group_id);
        if (!new_str) {
            return NULL;
        }

        if (!err) {
            cache_store_(group_cache, &group_index, (uint64_t)group_id,
                         new_str->str);
        }
    }

    return new_str;
}

static t_str *get_dt_(const t_alloc *alloc, const struct timespec *ctim) {
    Arena_Mark mark = {0};
    t_str *new_str;

    if (alloc->kind == ALLOC_ARENA) {
        mark = arena_get_mark(alloc->as.arena);
    }

    new_str = init_str(alloc, DT_LEN);
    if (!new_str) {
        return NULL;
    }

    const char *dt = ctime(&ctim->tv_sec);
    if (!dt) {
        free_alloc(alloc, mark, new_str, free_str_cb_);
        return NULL;
    }

    ft_memcpy(new_str->str, dt + 4, 7);
    ft_memcpy(new_str->str + 7, dt + 11, 5);
    new_str->len = 12;
    new_str->str[new_str->len] = '\0';

    return new_str;
}

static t_str *get_symlink_(const t_alloc *alloc, const t_entry *entry) {
    if (!S_ISLNK(entry->st.st_mode)) {
        return init_str(alloc, 1);
    }

    if (!entry->symlink_ready) {
        return NULL;
    }

    if (entry->symlink) {
        return dup_str(alloc, entry->symlink);
    }

    return init_str(alloc, 1);
}

static void free_info_(free_list *fl, t_file_info *info) {
    if (info->perm) {
        free_str(fl, info->perm);
    }

    if (info->links) {
        free_str(fl, info->links);
    }

    if (info->username) {
        free_str(fl, info->username);
    }

    if (info->groupname) {
        free_str(fl, info->groupname);
    }

    if (info->size) {
        free_str(fl, info->size);
    }

    if (info->dt) {
        free_str(fl, info->dt);
    }

    if (info->symlink) {
        free_str(fl, info->symlink);
    }

    fl_free(fl, info);
}

static void free_info_cb_(free_list *fl, void *ptr) {
    free_info_(fl, ptr);
}

static void free_entry_cb_(free_list *fl, void *ptr) {
    free_entry(fl, ptr);
}

static void free_str_cb_(free_list *fl, void *ptr) {
    free_str(fl, ptr);
}

static char *cache_lookup_(t_id_cache_entry *cache, uint64_t id) {
    for (uint64_t index = 0; index < CACHE_SIZE; ++index) {
        if (!*cache[index].name) {
            continue;
        }

        if (cache[index].id == id) {
            return cache[index].name;
        }
    }

    return NULL;
}

static void cache_store_(t_id_cache_entry *cache, uint64_t *next, uint64_t id,
                         const char *name) {
    const uint64_t index = *next % CACHE_SIZE;
    cache[index].id = id;
    ft_strlcpy(cache[index].name, name, LOGIN_NAME_MAX);
    ++(*next);
}
