#include <errno.h>
#include <grp.h>
#include <limits.h>
#include <pwd.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <time.h>

#include "../../libft/include/libft.h"

#include "./ft_arena.h"
#include "./ft_file_info.h"
#include "./ft_ls.h"
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

#ifndef XATTR_STACK_SIZE
#define XATTR_STACK_SIZE 1024
#endif /* ifndef XATTR_STACK_SIZE */

typedef struct {
    char name[LOGIN_NAME_MAX];
    uint64_t id;
} t_id_cache_entry;

static uint64_t user_index = 0;
static t_id_cache_entry user_cache[CACHE_SIZE] = {0};

static uint64_t group_index = 0;
static t_id_cache_entry group_cache[CACHE_SIZE] = {0};

static bool fill_file_info_(Arena *arena, t_file_info *info,
                            const t_entry *entry, time_t now, bool acces_time);
static t_str *get_perm_(Arena *arena, const t_entry *entry);
static char file_type_char_(mode_t mode);
static t_str *get_links_(Arena *arena, const t_entry *entry);
static t_str *get_user_(Arena *arena, const t_entry *entry);
static t_str *get_group_(Arena *arena, const t_entry *entry);
static t_str *get_size_(Arena *arena, const t_entry *entry);
static t_str *get_dt_(Arena *arena, time_t now, const t_entry *entry,
                      bool acces_time);
static char *cache_lookup_(t_id_cache_entry *cache, uint64_t id);
static void cache_store_(t_id_cache_entry *cache, uint64_t *next, uint64_t id,
                         const char *name);
static char exec_char_(mode_t mode, mode_t exec_bit, mode_t special_bit,
                       char lower, char upper);
static char get_acl_attr_(Arena *arena, const char *path);
static char classify_xattrs_(const char *names, ssize_t size);
static bool is_posix_acl_(const char *name);
static bool is_security_context_(const char *name);
static char xattr_error_marker_(int e);

bool prepare_list_infos(Arena *arena, const t_array *entries,
                        t_file_info **infos, const bool acces_time) {
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

        if (!fill_file_info_(arena, &(*infos)[index], entry, now, acces_time)) {
            return false;
        }
    }

    return true;
}

static bool fill_file_info_(Arena *arena, t_file_info *info,
                            const t_entry *entry, const time_t now,
                            const bool acces_time) {
    info->perm = get_perm_(arena, entry);
    if (!info->perm) {
        return false;
    }

    info->links = get_links_(arena, entry);
    if (!info->links) {
        return false;
    }

    info->username = get_user_(arena, entry);
    if (!info->username) {
        return false;
    }

    info->groupname = get_group_(arena, entry);
    if (!info->groupname) {
        return false;
    }

    info->size = get_size_(arena, entry);
    if (!info->size) {
        return false;
    }

    info->dt = get_dt_(arena, now, entry, acces_time);
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

    const char chr = get_acl_attr_(arena, entry->path->str);
    if (chr != ' ') {
        str->str[index++] = chr;
    }

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

static t_str *get_links_(Arena *arena, const t_entry *entry) {
    if (entry->stat_unavailable) {
        return str_arena_from_cstr(arena, "?");
    }

    return str_arena_from_uint(arena, (uint64_t)entry->st.st_nlink);
}

static t_str *get_user_(Arena *arena, const t_entry *entry) {
    if (entry->stat_unavailable) {
        return str_arena_from_cstr(arena, "?");
    }

    const uint64_t user_id = (uint64_t)entry->st.st_uid;
    const char *name = cache_lookup_(user_cache, user_id);
    if (name) {
        return str_arena_from_cstr(arena, name);
    }

    errno = 0;
    const struct passwd *pwd = getpwuid((uid_t)user_id);
    t_str *new_str = NULL;
    if (pwd) {
        new_str = str_arena_from_cstr(arena, pwd->pw_name);
        if (!new_str) {
            return NULL;
        }
        cache_store_(user_cache, &user_index, user_id, pwd->pw_name);
    } else {
        const int err = errno;

        new_str = str_arena_from_uint(arena, user_id);
        if (!new_str) {
            return NULL;
        }

        if (!err) {
            cache_store_(user_cache, &user_index, user_id, new_str->str);
        }
    }

    return new_str;
}

static t_str *get_group_(Arena *arena, const t_entry *entry) {
    if (entry->stat_unavailable) {
        return str_arena_from_cstr(arena, "?");
    }

    const uint64_t group_id = (uint64_t)entry->st.st_gid;
    const char *name = cache_lookup_(group_cache, group_id);
    if (name) {
        return str_arena_from_cstr(arena, name);
    }

    errno = 0;
    const struct group *grp = getgrgid((gid_t)group_id);
    t_str *new_str = NULL;
    if (grp) {
        new_str = str_arena_from_cstr(arena, grp->gr_name);
        if (!new_str) {
            return NULL;
        }

        cache_store_(group_cache, &group_index, group_id, grp->gr_name);
    } else {
        const int err = errno;

        new_str = str_arena_from_uint(arena, group_id);
        if (!new_str) {
            return NULL;
        }

        if (!err) {
            cache_store_(group_cache, &group_index, group_id, new_str->str);
        }
    }

    return new_str;
}

static t_str *get_size_(Arena *arena, const t_entry *entry) {
    if (entry->stat_unavailable) {
        return str_arena_from_cstr(arena, "?");
    }

    return str_arena_from_uint(arena, (uint64_t)entry->st.st_size);
}

static t_str *get_dt_(Arena *arena, time_t now, const t_entry *entry,
                      const bool acces_time) {
    if (entry->stat_unavailable) {
        return str_arena_from_cstr(arena, "           ?");
    }

    const Arena_Mark mark = arena_get_mark(arena);
    t_str *new_str = str_arena_new(arena, DT_LEN);
    if (!new_str) {
        return NULL;
    }

    const struct timespec *ctim;
    if (acces_time) {
        ctim = &entry->st.st_atim;
    } else {
        ctim = &entry->st.st_mtim;
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

static char get_acl_attr_(Arena *arena, const char *path) {
    errno = 0;
    char names[XATTR_STACK_SIZE];

    ssize_t size = llistxattr(path, names, sizeof(names));
    if (size == 0) {
        return ' ';
    }

    if (size > 0) {
        return classify_xattrs_(names, size);
    }

    if (errno != ERANGE) {
        return xattr_error_marker_(errno);
    }

    size = llistxattr(path, NULL, 0);
    if (size < 0) {
        return xattr_error_marker_(errno);
    }

    if (size == 0) {
        return ' ';
    }

    const Arena_Mark mark = arena_get_mark(arena);
    char *large_names = arena_push(arena, (size_t)size);
    if (!large_names) {
        arena_pop_to_mark(arena, mark);
        return '?';
    }

    size = llistxattr(path, large_names, (size_t)size);
    const char chr = size >= 0 ? classify_xattrs_(large_names, size)
                               : xattr_error_marker_(errno);

    arena_pop_to_mark(arena, mark);
    return chr;
}

static bool is_posix_acl_(const char *name) {
    return ft_strncmp(name, "system.posix_acl_access",
                      sizeof("system.posix_acl_access")) == 0 ||
           ft_strncmp(name, "system.posix_acl_default",
                      sizeof("system.posix_acl_default")) == 0;
}

static bool is_security_context_(const char *name) {
    return ft_strncmp(name, "security.selinux", sizeof("security.selinux")) ==
           0;
}

static char classify_xattrs_(const char *names, const ssize_t size) {
    bool security_context = false;
    const char *end = names + size;

    for (const char *p = names; p < end; p += ft_strlen(p) + 1) {
        if (is_posix_acl_(p)) {
            return '+';
        }

        if (is_security_context_(p)) {
            security_context = true;
        }
    }

    return security_context ? '.' : ' ';
}

static char xattr_error_marker_(const int e) {
    if (e == ENOTSUP || e == EOPNOTSUPP) {
        return ' ';
    }

    return '?';
}
