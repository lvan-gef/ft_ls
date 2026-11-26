#include <errno.h>
#include <grp.h>
#include <limits.h>
#include <pwd.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/xattr.h>
#include <time.h>

#include "./ft_arena.h"
#include "./ft_array.h"
#include "./ft_file_info.h"
#include "./ft_ls.h"
#include "./ft_printer.h"
#include "./ft_str_arena.h"
#include "./ft_symlink.h"
#include "./ft_utils.h"

struct timespec;

#ifndef CACHE_SIZE
#define CACHE_SIZE 64
#endif /* ifndef CACHE_SIZE */

#ifndef LOGIN_NAME_MAX
#define LOGIN_NAME_MAX 256
#endif /* ifndef LOGIN_NAME_MAX */

#ifndef LS_RECENT_SECS
#define LS_RECENT_SECS ((time_t)(31556952 / 2))
#endif /* ifndef LS_RECENT_SECS */

#ifndef XATTR_LIST_MAX
#define XATTR_LIST_MAX 65536
#endif /* ifndef XATTR_LIST_MAX */

typedef struct {
    char name[LOGIN_NAME_MAX];
    size_t id;
} t_id_cache_entry;

typedef struct {
    t_id_cache_entry entries[CACHE_SIZE];
    size_t next;
} t_id_cache;

typedef const char *(*t_id_lookup)(size_t id);

static t_id_cache user_cache = {0};
static t_id_cache group_cache = {0};

static bool fill_file_info_(const t_print_request *req, t_file_info *info,
                            const t_entry *entry, time_t now);
static void get_perm_(t_file_info *info, const t_entry *entry);
static char file_type_char_(mode_t mode);
static t_str *get_id_name_(t_arena *arena, t_id_cache *cache, size_t id,
                           t_id_lookup lookup);
static const char *user_name_(size_t id);
static const char *group_name_(size_t id);
static bool get_dt_(char *dt, const time_t now, const t_entry *entry,
                    bool acces_time);
static const char *cache_lookup_(const t_id_cache *cache, size_t id);
static void cache_store_(t_id_cache *cache, size_t id, const char *name);
static char exec_char_(mode_t mode, mode_t exec_bit, mode_t special_bit,
                       char lower, char upper);
static char get_acl_attr_(const char *path);
static char classify_xattrs_(const char *names, ssize_t size);
static bool is_posix_acl_(const char *name);
static bool is_security_context_(const char *name);
static char xattr_error_marker_(int e);

bool prepare_list_infos(const t_print_request *req, t_file_info **infos) {
    const t_array *entries = req->entries;
    if (!entries || !entries->len) {
        *infos = NULL;
        return true;
    }

    const time_t now = time(NULL);
    if (now == (time_t)-1) {
        return false;
    }

    *infos = arena_push(req->ctx->arena, sizeof(**infos) * entries->len);
    if (!*infos) {
        return false;
    }

    for (size_t index = 0; index < entries->len; ++index) {
        const t_entry *entry = entries->data[index];

        if (!fill_file_info_(req, &(*infos)[index], entry, now)) {
            return false;
        }
    }

    return true;
}

static bool fill_file_info_(const t_print_request *req, t_file_info *info,
                            const t_entry *entry, const time_t now) {
    *info = (t_file_info){0};
    const t_args *args = req->ctx->args;
    t_arena *arena = req->ctx->arena;
    const bool unknown = entry->stat_unavailable;

    get_perm_(info, entry);
    if (!args->no_owner) {
        info->username = unknown ? str_arena_from_cstr(arena, "?")
                                 : get_id_name_(arena, &user_cache,
                                                entry->st.st_uid, user_name_);
        if (!info->username) {
            return false;
        }
    }
    if (!args->no_group) {
        info->groupname = unknown ? str_arena_from_cstr(arena, "?")
                                  : get_id_name_(arena, &group_cache,
                                                 entry->st.st_gid, group_name_);
        if (!info->groupname) {
            return false;
        }
    }

    if (!get_dt_(info->dt, now, entry, args->access_time)) {
        return false;
    }

    if (S_ISLNK(entry->st.st_mode)) {
        info->symlink =
            read_symlink(arena, entry->path, (size_t)entry->st.st_size, NULL);
        if (!info->symlink) {
            return false;
        }
    } else {
        info->symlink = NULL;
    }

    return true;
}

static void get_perm_(t_file_info *info, const t_entry *entry) {
    size_t index = 0;
    char *p = info->perm;
    p[index++] = file_type_char_(entry->st.st_mode);
    if (entry->stat_unavailable) {
        while (index < 10) {
            p[index++] = '?';
        }

        p[index] = '\0';
        info->perm_len = index;
        return;
    }

    p[index++] = entry->st.st_mode & S_IRUSR ? 'r' : '-';
    p[index++] = entry->st.st_mode & S_IWUSR ? 'w' : '-';
    p[index++] = exec_char_(entry->st.st_mode, S_IXUSR, S_ISUID, 's', 'S');
    p[index++] = entry->st.st_mode & S_IRGRP ? 'r' : '-';
    p[index++] = entry->st.st_mode & S_IWGRP ? 'w' : '-';
    p[index++] = exec_char_(entry->st.st_mode, S_IXGRP, S_ISGID, 's', 'S');
    p[index++] = entry->st.st_mode & S_IROTH ? 'r' : '-';
    p[index++] = entry->st.st_mode & S_IWOTH ? 'w' : '-';
    p[index++] = exec_char_(entry->st.st_mode, S_IXOTH, S_ISVTX, 't', 'T');

    const char chr = get_acl_attr_(entry->path->str);
    if (chr != ' ') {
        p[index++] = chr;
    }

    p[index] = '\0';
    info->perm_len = index;
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

static t_str *get_id_name_(t_arena *arena, t_id_cache *cache, const size_t id,
                           const t_id_lookup lookup) {
    const char *name = cache_lookup_(cache, id);
    if (name) {
        return str_arena_from_cstr(arena, name);
    }

    errno = 0;
    name = lookup(id);
    const int err = errno;

    t_str *str = name ? str_arena_from_cstr(arena, name)
                      : str_arena_from_uint(arena, id);
    if (str && (name || !err)) {
        cache_store_(cache, id, str->str);
    }

    return str;
}

static const char *user_name_(const size_t id) {
    const struct passwd *pwd = getpwuid((uid_t)id);
    return pwd ? pwd->pw_name : NULL;
}

static const char *group_name_(const size_t id) {
    const struct group *grp = getgrgid((gid_t)id);
    return grp ? grp->gr_name : NULL;
}

static bool get_dt_(char *dt, const time_t now, const t_entry *entry,
                    const bool acces_time) {
    if (entry->stat_unavailable) {
        ft_memcpy(dt, "           ?", DT_LEN);
        return true;
    }

    const time_t stamp =
        acces_time ? entry->st.st_atim.tv_sec : entry->st.st_mtim.tv_sec;
    const char *s = ctime(&stamp);
    if (!s) {
        return false;
    }

    bool recent = stamp <= now && now - stamp < LS_RECENT_SECS;
    if (recent) {
        ft_memcpy(dt, s + 4, DT_LEN - 1);
    } else {
        ft_memcpy(dt, s + 4, 7);
        dt[7] = ' ';
        ft_memcpy(dt + 8, s + 20, 4);
    }

    dt[DT_LEN - 1] = '\0';
    return true;
}

static const char *cache_lookup_(const t_id_cache *cache, const size_t id) {
    for (size_t index = 0; index < CACHE_SIZE; ++index) {
        const t_id_cache_entry *slot = &cache->entries[index];
        if (*slot->name && slot->id == id) {
            return slot->name;
        }
    }

    return NULL;
}

static void cache_store_(t_id_cache *cache, const size_t id, const char *name) {
    t_id_cache_entry *slot = &cache->entries[cache->next % CACHE_SIZE];

    slot->id = id;
    ft_strlcpy(slot->name, name, LOGIN_NAME_MAX);
    ++cache->next;
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

static char get_acl_attr_(const char *path) {
    static char names[XATTR_LIST_MAX];

    const ssize_t size = llistxattr(path, names, sizeof(names));
    if (size == 0) {
        return ' ';
    }

    if (size > 0) {
        return classify_xattrs_(names, size);
    }

    return xattr_error_marker_(errno);
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
    if (e == ENOTSUP) {
        return ' ';
    }

    return '?';
}
