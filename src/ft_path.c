#include <errno.h>
#include <grp.h>
#include <linux/limits.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <time.h>

#include "../include/ft_arena.h"
#include "../include/ft_assert.h"
#include "../include/ft_path.h"
#include "../include/ft_str.h"

#include "../libft/include/libft.h"

static t_str *get_perm_(Arena *arena, Arena *scratch, const t_entry *entry);
static t_str *get_user_(Arena *arena, uid_t user_id);
static t_str *get_group_(Arena *arena, gid_t group_id);
static t_str *get_dt_(Arena *arena, const struct timespec *ctim);
static bool get_symlink_(Arena *arena, const t_entry *entry, t_str **out);
static bool has_xattr_(const char *path, const char *name);

typedef enum {
    P_LINK,
    P_REG,
    P_DIR,
    P_RUSER,
    P_WUSER,
    P_XUSER,
    P_RGROUP,
    P_WGROUP,
    P_XGROUP,
    P_ROTHER,
    P_WOTHER,
    P_XOTHER,
    P_ATTR,
    P_TOTAL
} t_perm_lttr;

static uid_t cached_uid = (uid_t)-1;
static gid_t cached_gid = (gid_t)-1;
static char cached_user[LOGIN_NAME_MAX] = "";
static char cached_group[LOGIN_NAME_MAX] = "";

bool get_file_info(Arena *arena, Arena *scratch, t_entry *entry) {
    ASSERT_NOTNULL(entry);
    ASSERT_NOTNULL(entry->path);
    ASSERT_GT(entry->path->cap, 0);
    ASSERT_LT(entry->path->len, entry->path->cap);
    ASSERT_NOTNULL(arena);
    ASSERT_NOTNULL(scratch);

    const ArenaMark mark = ArenaGetMark(arena);
    ArenaClear(scratch);

    t_file_info *info = ArenaPush(arena, sizeof(*info));
    if (!info) {
        goto failed;
    }

    info->perm = get_perm_(arena, scratch, entry);
    if (!info->perm) {
        goto failed;
    }
    ArenaClear(scratch);

    info->links = uint_to_str(arena, entry->st.st_nlink);
    if (!info->links) {
        goto failed;
    }

    info->username = get_user_(arena, entry->st.st_uid);
    if (!info->username) {
        goto failed;
    }

    info->groupname = get_group_(arena, entry->st.st_gid);
    if (!info->groupname) {
        goto failed;
    }

    info->size = uint_to_str(arena, (uint64_t)entry->st.st_size);
    if (!info->size) {
        goto failed;
    }

    info->dt = get_dt_(arena, &entry->st.st_mtim);
    if (!info->dt) {
        goto failed;
    }

    if (!get_symlink_(arena, entry, &info->symlink)) {
        goto failed;
    }

    info->blocks = (uint64_t)entry->st.st_blocks;
    ArenaClear(scratch);

    entry->info = info;
    return true;

failed:
    ArenaClear(scratch);
    ArenaPopToMark(arena, mark);

    return false;
}

static t_str *get_perm_(Arena *arena, Arena *scratch, const t_entry *entry) {
    const ArenaMark mark = ArenaGetMark(arena);

    t_str *str = init_str(arena, PERMISSION_SIZE);
    if (!str) {
        goto failed;
    }

    for (uint64_t index = 0; index < P_TOTAL; ++index) {
        switch (index) {
            case P_LINK:
                if ((entry->st.st_mode & S_IFMT) == S_IFLNK) {
                    append_chars_str(scratch, str, "l");
                }
                break;
            case P_REG:
                if ((entry->st.st_mode & S_IFMT) == S_IFREG) {
                    append_chars_str(scratch, str, "-");
                }
                break;

            case P_DIR:
                if ((entry->st.st_mode & S_IFMT) == S_IFDIR) {
                    append_chars_str(scratch, str, "d");
                }
                break;
            case P_RUSER:
                append_chars_str(scratch, str,
                                 entry->st.st_mode & S_IRUSR ? "r" : "-");
                break;
            case P_WUSER:
                append_chars_str(scratch, str,
                                 entry->st.st_mode & S_IWUSR ? "w" : "-");
                break;
            case P_XUSER:
                append_chars_str(scratch, str,
                                 entry->st.st_mode & S_IXUSR ? "x" : "-");
                break;
            case P_RGROUP:
                append_chars_str(scratch, str,
                                 entry->st.st_mode & S_IRGRP ? "r" : "-");
                break;
            case P_WGROUP:
                append_chars_str(scratch, str,
                                 entry->st.st_mode & S_IWGRP ? "w" : "-");
                break;
            case P_XGROUP:
                append_chars_str(scratch, str,
                                 entry->st.st_mode & S_IXGRP ? "x" : "-");
                break;
            case P_ROTHER:
                append_chars_str(scratch, str,
                                 entry->st.st_mode & S_IROTH ? "r" : "-");
                break;
            case P_WOTHER:
                append_chars_str(scratch, str,
                                 entry->st.st_mode & S_IWOTH ? "w" : "-");
                break;
            case P_XOTHER:
                append_chars_str(scratch, str,
                                 entry->st.st_mode & S_IXOTH ? "x" : "-");
                break;
            case P_ATTR:
                if (has_xattr_(entry->path->str, "system.posix_acl_access")) {
                    append_chars_str(scratch, str, "+");
                } else if (has_xattr_(entry->path->str, "security.selinux")) {
                    append_chars_str(scratch, str, ".");
                }
                break;
            default: ASSERT_FALSE(true);
        }
    }

    return str;
failed:
    ArenaPopToMark(arena, mark);

    return NULL;
}

static t_str *get_user_(Arena *arena, uid_t user_id) {
    if (user_id == cached_uid) {
        return create_str(arena, cached_user);
    }

    const struct passwd *pwd = getpwuid(user_id);
    t_str *new_str = NULL;
    if (pwd) {
        new_str = create_str(arena, pwd->pw_name);
        if (!new_str) {
            return NULL;
        }

        ft_strlcpy(cached_user, pwd->pw_name, sizeof(cached_user));
    } else {
        new_str = uint_to_str(arena, user_id);
        if (!new_str) {
            return NULL;
        }

        ft_strlcpy(cached_user, new_str->str, sizeof(cached_user));
    }

    cached_uid = user_id;
    return new_str;
}

static t_str *get_group_(Arena *arena, gid_t group_id) {
    if (group_id == cached_gid) {
        return create_str(arena, cached_group);
    }

    const struct group *grp = getgrgid(group_id);
    t_str *new_str = NULL;
    if (grp) {
        new_str = create_str(arena, grp->gr_name);
        if (!new_str) {
            return NULL;
        }

        ft_strlcpy(cached_group, grp->gr_name, sizeof(cached_group));
    } else {
        new_str = uint_to_str(arena, group_id);
        if (!new_str) {
            return NULL;
        }

        ft_strlcpy(cached_group, new_str->str, sizeof(cached_group));
    }

    cached_gid = group_id;
    return new_str;
}

static t_str *get_dt_(Arena *arena, const struct timespec *ctim) {
    const char *dt = ctime(&ctim->tv_sec);
    if (!dt) {
        return NULL;
    }

    const size_t len = ft_strlen(dt);
    if (len < 16) {
        return NULL;
    }

    t_str *new_str = init_str(arena, DT_LEN);
    if (!new_str) {
        return NULL;
    }

    ft_memcpy(new_str->str, dt + 4, 7);
    ft_memcpy(new_str->str + 7, dt + 11, 5);
    new_str->len = 12;
    new_str->str[new_str->len] = '\0';

    ASSERT_EQ(new_str->len, DT_LEN - 1);
    return new_str;
}

static bool get_symlink_(Arena *arena, const t_entry *entry, t_str **out) {
    ASSERT_NOTNULL(arena);
    ASSERT_NOTNULL(entry);
    ASSERT_NOTNULL(out);

    *out = NULL;
    if (!S_ISLNK(entry->st.st_mode)) {
        return true;
    }

    ArenaMark mark = ArenaGetMark(arena);
    uint64_t cap = (entry->st.st_size > 0) ? (uint64_t)entry->st.st_size
                                           : (uint64_t)PATH_MAX;
    while (true) {
        t_str *new_str = init_str(arena, cap);
        if (!new_str) {
            goto failed;
        }

        ssize_t len = readlink(entry->path->str, new_str->str, (size_t)cap);
        if (len < 0) {
            goto failed;
        }

        if ((uint64_t)len < cap) {
            new_str->len = (uint64_t)len;
            new_str->str[new_str->len] = '\0';
            *out = new_str;
            return true;
        }

        ArenaPopToMark(arena, mark);
        if (cap > UINT64_MAX / 2) {
            goto failed;
        }

        cap *= 2;
    }

failed:
    ArenaPopToMark(arena, mark);
    return false;
}

static bool has_xattr_(const char *path, const char *name) {
    ssize_t n = getxattr(path, name, NULL, 0);
    if (n < 0) {
        return false;
    }

    return true;
}
