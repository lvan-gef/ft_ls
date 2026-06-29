#include <errno.h>
#include <grp.h>
#include <limits.h>
#include <pwd.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
#include <time.h>

#include "../libft/include/libft.h"

#include "./ft_arena.h"
#include "./ft_entry.h"
#include "./ft_file_info.h"
#include "./ft_str_arena.h"
#include "./ft_symlink.h"

struct timespec;

#ifndef CACHE_SIZE
#define CACHE_SIZE UINT64_C(64)
#endif /* ifndef CACHE_SIZE */

#ifndef PERMISSION_SIZE
#define PERMISSION_SIZE UINT64_C(12)
#endif /* ifndef PERMISSION_SIZE */

#ifndef DT_LEN
#define DT_LEN UINT64_C(13)
#endif /* ifndef DT_LEN */

#ifndef LOGIN_NAME_MAX
#define LOGIN_NAME_MAX INT64_C(256)
#endif /* ifndef LOGIN_NAME_MAX */

#ifndef LS_RECENT_SECS
#define LS_RECENT_SECS ((time_t)(31556952 / 2))
#endif /* ifndef LS_RECENT_SECS */

typedef struct {
    char name[LOGIN_NAME_MAX];
    uint64_t id;
} t_id_cache_entry;

static uint64_t user_index = 0;
static t_id_cache_entry user_cache[CACHE_SIZE] = {0};

static uint64_t group_index = 0;
static t_id_cache_entry group_cache[CACHE_SIZE] = {0};

static bool fill_file_info_(Arena *arena, t_file_info *info,
                            const t_entry *entry, time_t now);
static t_str *get_perm_(Arena *arena, const t_entry *entry);
static t_str *get_user_(Arena *arena, uid_t user_id);
static t_str *get_group_(Arena *arena, gid_t group_id);
static t_str *get_dt_(Arena *arena, const struct timespec *ctim, time_t now);
static char *cache_lookup_(t_id_cache_entry *cache, uint64_t id);
static void cache_store_(t_id_cache_entry *cache, uint64_t *next, uint64_t id,
                         const char *name);
static char file_type_char_(mode_t mode);
static char exec_char_(mode_t mode, mode_t exec_bit, mode_t special_bit,
                       char lower, char upper);

bool prepare_list_infos(Arena *arena, const t_array *entries,
                        t_file_info **infos) {
    if (!entries || !entries->len) {
        *infos = NULL;
        return true;
    }

    const time_t now = time(NULL);
    if (now == (time_t)-1) {
        return false;
    }

    *infos = arena_push(arena, sizeof(**infos) * entries->len);
    if (!*infos) {
        return false;
    }

    for (uint64_t index = 0; index < entries->len; ++index) {
        const t_entry *entry = entries->data[index];

        if (!fill_file_info_(arena, &(*infos)[index], entry, now)) {
            return false;
        }
    }

    return true;
}

static bool fill_file_info_(Arena *arena, t_file_info *info,
                            const t_entry *entry, const time_t now) {
    if (entry->stat_unavailable) {
        info->perm = get_perm_(arena, entry);
        if (!info->perm) {
            return false;
        }

        info->links = str_arena_from_cstr(arena, "?");
        if (!info->links) {
            return false;
        }

        info->username = str_arena_from_cstr(arena, "?");
        if (!info->username) {
            return false;
        }

        info->groupname = str_arena_from_cstr(arena, "?");
        if (!info->groupname) {
            return false;
        }

        info->size = str_arena_from_cstr(arena, "?");
        if (!info->size) {
            return false;
        }

        info->dt = str_arena_from_cstr(arena, "           ?");
        if (!info->dt) {
            return false;
        }

        info->symlink = NULL;
        info->blocks = 0;
        return true;
    }

    info->perm = get_perm_(arena, entry);
    if (!info->perm) {
        return false;
    }

    info->links = str_arena_from_uint(arena, entry->st.st_nlink);
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

    info->size = str_arena_from_uint(arena, (uint64_t)entry->st.st_size);
    if (!info->size) {
        return false;
    }

    info->dt = get_dt_(arena, &entry->st.st_mtim, now);
    if (!info->dt) {
        return false;
    }

    if (S_ISLNK(entry->st.st_mode)) {
        info->symlink =
            read_symlink(arena, entry->path, (uint64_t)entry->st.st_size, NULL);
        if (!info->symlink) {
            return false;
        }
    } else {
        info->symlink = NULL;
    }

    info->blocks = (uint64_t)entry->st.st_blocks;
    return true;
}

static t_str *get_perm_(Arena *arena, const t_entry *entry) {
    t_str *str = str_arena_new(arena, PERMISSION_SIZE);
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
        return str_arena_from_cstr(arena, name);
    }

    errno = 0;
    const struct passwd *pwd = getpwuid(user_id);
    t_str *new_str = NULL;
    if (pwd) {
        new_str = str_arena_from_cstr(arena, pwd->pw_name);
        if (!new_str) {
            return NULL;
        }
        cache_store_(user_cache, &user_index, (uint64_t)user_id, pwd->pw_name);
    } else {
        const int err = errno;

        new_str = str_arena_from_uint(arena, (uint64_t)user_id);
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
        return str_arena_from_cstr(arena, name);
    }

    errno = 0;
    const struct group *grp = getgrgid(group_id);
    t_str *new_str = NULL;
    if (grp) {
        new_str = str_arena_from_cstr(arena, grp->gr_name);
        if (!new_str) {
            return NULL;
        }

        cache_store_(group_cache, &group_index, (uint64_t)group_id,
                     grp->gr_name);
    } else {
        const int err = errno;

        new_str = str_arena_from_uint(arena, (uint64_t)group_id);
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

static t_str *get_dt_(Arena *arena, const struct timespec *ctim,
                      const time_t now) {
    const Arena_Mark mark = arena_get_mark(arena);
    t_str *new_str = str_arena_new(arena, DT_LEN);
    if (!new_str) {
        return NULL;
    }

    const time_t stamp = ctim->tv_sec;
    const char *dt = ctime(&stamp);
    if (!dt) {
        arena_pop_to_mark(arena, mark);
        return NULL;
    }

    bool recent = false;
    if (stamp <= now) {
        recent = (now - stamp) < LS_RECENT_SECS;
    }

    if (recent) {
        ft_memcpy(new_str->str, dt + 4, DT_LEN - 1);
    } else {
        ft_memcpy(new_str->str, dt + 4, 7);
        new_str->str[7] = ' ';
        ft_memcpy(new_str->str + 8, dt + 20, 4);
    }

    new_str->len = DT_LEN - 1;
    new_str->str[DT_LEN - 1] = '\0';
    return new_str;
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
