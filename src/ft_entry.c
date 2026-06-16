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

#ifndef CACHE_SIZE
#define CACHE_SIZE UINT64_C(8)
#endif // ifndef CACHE_SIZE //

#ifndef PERMISSION_SIZE
#define PERMISSION_SIZE UINT64_C(12)
#endif // ifndef PERMISSION_SIZE //

#ifndef DT_LEN
#define DT_LEN UINT64_C(13)
#endif // ifndef DT_LEN //

#ifndef LOGIN_NAME_MAX
#define LOGIN_NAME_MAX INT64_C(256)
#endif // ifndef LOGIN_NAME_MAX //

#ifndef PATH_MAX
#define PATH_MAX INT64_C(4096)
#endif // ifndef PATH_MAX //

#ifndef LS_RECENT_SECS
#define LS_RECENT_SECS ((time_t)(31556952 / 2))
#endif // ifndef LS_RECENT_SECS //

typedef struct {
    char name[LOGIN_NAME_MAX];
    uint64_t id;
} t_id_cache_entry;

static uint64_t user_index = 0;
static t_id_cache_entry user_cache[CACHE_SIZE] = {0};

static uint64_t group_index = 0;
static t_id_cache_entry group_cache[CACHE_SIZE] = {0};

static bool fill_file_info_(Arena *arena, t_file_info *info,
                            const t_entry *entry);
static t_str *arena_join_dir_path_(Arena *arena, const t_str *lhs,
                                   const t_str *rhs);
static t_str *fl_join_dir_path_(free_list *fl, const t_str *lhs,
                                const t_str *rhs);
static void fill_join_dir(t_str *path, const t_str *lhs, const t_str *rhs,
                          bool need_slash);
static t_str *unknown_field_(Arena *arena);
static t_str *unknown_dt_field_(Arena *arena);
static t_str *get_perm_(Arena *arena, const t_entry *entry);
static t_str *get_user_(Arena *arena, uid_t user_id);
static t_str *get_group_(Arena *arena, gid_t group_id);
static t_str *get_dt_(Arena *arena, const struct timespec *ctim);
static t_str *get_symlink_(Arena *arena, const t_entry *entry);
static char *cache_lookup_(t_id_cache_entry *cache, uint64_t id);
static void cache_store_(t_id_cache_entry *cache, uint64_t *next, uint64_t id,
                         const char *name);
static char file_type_char_(mode_t mode);
static char exec_char_(mode_t mode, mode_t exec_bit, mode_t special_bit,
                       char lower, char upper);

t_entry *new_scanned_entry(Arena *arena, const t_entry *parent,
                           const struct dirent *dp) {
    Arena_Mark mark = {0};
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
    ent->info = NULL;
    ent->st = (struct stat){0};
    return ent;
failed:
    arena_pop_to_mark(arena, mark);
    return NULL;
}

bool fill_file_info(Arena *arena, t_entry *entry) {
    Arena_Mark mark = arena_get_mark(arena);
    t_file_info *info = arena_push(arena, sizeof(*info));
    if (!info) {
        goto failed;
    }

    *info = (t_file_info){0};
    if (!fill_file_info_(arena, info, entry)) {
        goto failed;
    }

    entry->info = info;
    return true;

failed:
    arena_pop_to_mark(arena, mark);
    entry->info = NULL;
    return false;
}

bool ensure_entry_path(Arena *arena, t_entry *entry) {
    if (entry->path) {
        return true;
    }

    if (!entry->parent_path || !entry->name) {
        return false;
    }

    entry->path = arena_join_dir_path_(arena, entry->parent_path, entry->name);
    return entry->path != NULL;
}

t_entry *dup_dir_entry(free_list *fl, const t_entry *src,
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
        entry->path = dup_str(fl, src->path);
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

t_str *read_symlink_target(Arena *arena, const t_str *path,
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

    fl_free(fl, entry);
}

static bool fill_file_info_(Arena *arena, t_file_info *info,
                            const t_entry *entry) {
    if (entry->stat_unavailable) {
        info->perm = get_perm_(arena, entry);
        if (!info->perm) {
            return false;
        }

        info->links = unknown_field_(arena);
        if (!info->links) {
            return false;
        }

        info->username = unknown_field_(arena);
        if (!info->username) {
            return false;
        }

        info->groupname = unknown_field_(arena);
        if (!info->groupname) {
            return false;
        }

        info->size = unknown_field_(arena);
        if (!info->size) {
            return false;
        }

        info->dt = unknown_dt_field_(arena);
        if (!info->dt) {
            return false;
        }

        info->symlink = arena_init_str(arena, 1);
        if (!info->symlink) {
            return false;
        }

        info->blocks = 0;
        return true;
    }

    info->perm = get_perm_(arena, entry);
    if (!info->perm) {
        return false;
    }

    info->links = uint_to_str(arena, entry->st.st_nlink);
    if (!info->links) {
        return false;
    }

    info->username = get_user_(arena, entry->st.st_uid);
    if (!info->username) {
        return false;
    }

    info->groupname = get_group_(arena, entry->st.st_gid);
    if (!info->groupname) {
        return false;
    }

    info->size = uint_to_str(arena, (uint64_t)entry->st.st_size);
    if (!info->size) {
        return false;
    }

    info->dt = get_dt_(arena, &entry->st.st_mtim);
    if (!info->dt) {
        return false;
    }

    info->symlink = get_symlink_(arena, entry);
    if (!info->symlink) {
        return false;
    }

    info->blocks = (uint64_t)entry->st.st_blocks;
    return true;
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

static t_str *unknown_field_(Arena *arena) {
    return arena_create_str(arena, "?");
}

static t_str *unknown_dt_field_(Arena *arena) {
    return arena_create_str(arena, "           ?");
}

static t_str *get_perm_(Arena *arena, const t_entry *entry) {
    t_str *str = arena_init_str(arena, PERMISSION_SIZE);

    if (!str) {
        return NULL;
    }

    uint64_t index = 0;
    str->str[index++] = file_type_char_(entry->st.st_mode);
    if (entry->stat_unavailable) {
        while (index < 10) {
            str->str[index++] = '?';
        }

        str->len = index;
        str->str[index] = '\0';
        return str;
    }

    str->str[index++] = entry->st.st_mode & S_IRUSR ? 'r' : '-';
    str->str[index++] = entry->st.st_mode & S_IWUSR ? 'w' : '-';
    str->str[index++] =
        exec_char_(entry->st.st_mode, S_IXUSR, S_ISUID, 's', 'S');
    str->str[index++] = entry->st.st_mode & S_IRGRP ? 'r' : '-';
    str->str[index++] = entry->st.st_mode & S_IWGRP ? 'w' : '-';
    str->str[index++] =
        exec_char_(entry->st.st_mode, S_IXGRP, S_ISGID, 's', 'S');
    str->str[index++] = entry->st.st_mode & S_IROTH ? 'r' : '-';
    str->str[index++] = entry->st.st_mode & S_IWOTH ? 'w' : '-';
    str->str[index++] =
        exec_char_(entry->st.st_mode, S_IXOTH, S_ISVTX, 't', 'T');

    str->len = index;
    str->str[index] = '\0';
    return str;
}

static char file_type_char_(const mode_t mode) {
    if ((mode & S_IFMT) == S_IFREG) {
        return '-';
    }

    if ((mode & S_IFMT) == S_IFDIR) {
        return 'd';
    }

    if ((mode & S_IFMT) == S_IFLNK) {
        return 'l';
    }

    if ((mode & S_IFMT) == S_IFIFO) {
        return 'p';
    }

    if ((mode & S_IFMT) == S_IFSOCK) {
        return 's';
    }

    if ((mode & S_IFMT) == S_IFCHR) {
        return 'c';
    }

    if ((mode & S_IFMT) == S_IFBLK) {
        return 'b';
    }

    return '?';
}

static t_str *get_user_(Arena *arena, const uid_t user_id) {
    const char *name = cache_lookup_(user_cache, (uint64_t)user_id);
    if (name) {
        return arena_create_str(arena, name);
    }

    const struct passwd *pwd = getpwuid(user_id);
    t_str *new_str = NULL;
    if (pwd) {
        new_str = arena_create_str(arena, pwd->pw_name);
        if (!new_str) {
            return NULL;
        }
        cache_store_(user_cache, &user_index, (uint64_t)user_id, pwd->pw_name);
    } else {
        const int err = errno;

        new_str = uint_to_str(arena, (uint64_t)user_id);
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

static t_str *get_group_(Arena *arena, const gid_t group_id) {
    const char *name = cache_lookup_(group_cache, (uint64_t)group_id);
    if (name) {
        return arena_create_str(arena, name);
    }

    const struct group *grp = getgrgid(group_id);
    t_str *new_str = NULL;
    if (grp) {
        new_str = arena_create_str(arena, grp->gr_name);
        if (!new_str) {
            return NULL;
        }

        cache_store_(group_cache, &group_index, (uint64_t)group_id,
                     grp->gr_name);
    } else {
        const int err = errno;

        new_str = uint_to_str(arena, (uint64_t)group_id);
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

static t_str *get_dt_(Arena *arena, const struct timespec *ctim) {
    Arena_Mark mark = arena_get_mark(arena);
    t_str *new_str = arena_init_str(arena, DT_LEN);
    if (!new_str) {
        return NULL;
    }

    const time_t stamp = ctim->tv_sec;
    const char *dt = ctime(&stamp);
    if (!dt) {
        arena_pop_to_mark(arena, mark);
        return NULL;
    }

    const time_t now = time(NULL);
    if (now == (time_t)-1) {
        arena_pop_to_mark(arena, mark);
        return NULL;
    }

    bool recent = false;
    if (stamp <= now) {
        recent = (now - stamp) < LS_RECENT_SECS;
    }

    if (recent) {
        ft_memcpy(new_str->str, dt + 4, 12);
    } else {
        ft_memcpy(new_str->str, dt + 4, 7);
        new_str->str[7] = ' ';
        ft_memcpy(new_str->str + 8, dt + 20, 4);
    }

    new_str->len = 12;
    new_str->str[12] = '\0';
    return new_str;
}

static t_str *get_symlink_(Arena *arena, const t_entry *entry) {
    if (!S_ISLNK(entry->st.st_mode)) {
        return arena_init_str(arena, 1);
    }

    if (!entry->path) {
        return NULL;
    }

    return read_symlink_target(arena, entry->path, (uint64_t)entry->st.st_size,
                               NULL);
}

static char *cache_lookup_(t_id_cache_entry *cache, const uint64_t id) {
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

static void cache_store_(t_id_cache_entry *cache, uint64_t *next,
                         const uint64_t id, const char *name) {
    const uint64_t index = *next % CACHE_SIZE;
    cache[index].id = id;
    ft_strlcpy(cache[index].name, name, LOGIN_NAME_MAX);
    ++(*next);
}

static char exec_char_(const mode_t mode, const mode_t exec_bit,
                       const mode_t special_bit, const char lower,
                       const char upper) {
    const bool has_exec = (mode & exec_bit) != 0;
    const bool has_special = (mode & special_bit) != 0;

    if (!has_special) {
        return has_exec ? 'x' : '-';
    }

    return has_exec ? lower : upper;
}
