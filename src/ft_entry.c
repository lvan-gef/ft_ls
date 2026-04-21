#include <grp.h>
#include <linux/limits.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <time.h>

#include "../include/ft_assert.h"
#include "../include/ft_entry.h"
#include "../include/ft_free_list.h"
#include "../include/ft_str.h"

#include "../libft/include/libft.h"
#include "ft_shell_escape.h"

static t_str *get_perm_(free_list *fl, const t_entry *entry);
static t_str *get_user_(free_list *fl, uid_t user_id);
static t_str *get_group_(free_list *fl, gid_t group_id);
static t_str *get_dt_(free_list *fl, const struct timespec *ctim);
static bool get_symlink_(free_list *fl, const t_entry *entry, t_str **out);
static bool has_xattr_(const char *path, const char *name);
static t_str *join_paths_(free_list *fl, const t_str *lhs, const t_str *rhs);

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

t_entry *new_entry(free_list *fl, t_entry *entry, const struct dirent *dp) {
    ASSERT_NOTNULL(fl);
    ASSERT_NOTNULL(entry);
    ASSERT_NOTNULL(dp);

    t_entry *ent = fl_alloc(fl, sizeof(*entry), 8);
    if (!ent) {
        goto failed;
    }

    ent->name = create_str(fl, dp->d_name);
    if (!ent->name) {
        goto failed;
    }

    ent->path = join_paths_(fl, entry->path, ent->name);
    if (!ent->path) {
        goto failed;
    }

    ent->quote = shell_quote_style(ent->name);
    ent->is_escaped = false;
    ent->is_operand = false;

    return ent;
failed:
    return NULL;
}

bool get_file_info(free_list *fl, t_entry *entry) {
    ASSERT_NOTNULL(fl);
    ASSERT_NOTNULL(entry);
    ASSERT_NOTNULL(entry->path);
    ASSERT_GT(entry->path->cap, 0);
    ASSERT_LT(entry->path->len, entry->path->cap);

    t_file_info *info = fl_alloc(fl, sizeof(*info), 8);
    if (!info) {
        return false;
    }

    info->perm = get_perm_(fl, entry);
    if (!info->perm) {
        return false;
    }

    info->links = uint_to_str(fl, entry->st.st_nlink);
    if (!info->links) {
        return false;
    }

    info->username = get_user_(fl, entry->st.st_uid);
    if (!info->username) {
        return false;
    }

    info->groupname = get_group_(fl, entry->st.st_gid);
    if (!info->groupname) {
        return false;
    }

    info->size = uint_to_str(fl, (uint64_t)entry->st.st_size);
    if (!info->size) {
        return false;
    }

    info->dt = get_dt_(fl, &entry->st.st_mtim);
    if (!info->dt) {
        return false;
    }

    info->symlink = info->perm;
    if (!get_symlink_(fl, entry, &info->symlink)) {
        return false;
    }

    info->blocks = (uint64_t)entry->st.st_blocks;

    entry->info = info;
    return true;
}

static t_str *get_perm_(free_list *fl, const t_entry *entry) {
    ASSERT_NOTNULL(fl);
    ASSERT_NOTNULL(entry);

    t_str *str = init_str(fl, PERMISSION_SIZE);
    if (!str) {
        goto failed;
    }

    for (uint64_t index = 0; index < P_TOTAL; ++index) {
        switch (index) {
            case P_LINK:
                if ((entry->st.st_mode & S_IFMT) == S_IFLNK) {
                    append_chars_str(str, "l");
                }
                break;
            case P_REG:
                if ((entry->st.st_mode & S_IFMT) == S_IFREG) {
                    append_chars_str(str, "-");
                }
                break;
            case P_DIR:
                if ((entry->st.st_mode & S_IFMT) == S_IFDIR) {
                    append_chars_str(str, "d");
                }
                break;
            case P_RUSER:
                append_chars_str(str, entry->st.st_mode & S_IRUSR ? "r" : "-");
                break;
            case P_WUSER:
                append_chars_str(str, entry->st.st_mode & S_IWUSR ? "w" : "-");
                break;
            case P_XUSER:
                append_chars_str(str, entry->st.st_mode & S_IXUSR ? "x" : "-");
                break;
            case P_RGROUP:
                append_chars_str(str, entry->st.st_mode & S_IRGRP ? "r" : "-");
                break;
            case P_WGROUP:
                append_chars_str(str, entry->st.st_mode & S_IWGRP ? "w" : "-");
                break;
            case P_XGROUP:
                append_chars_str(str, entry->st.st_mode & S_IXGRP ? "x" : "-");
                break;
            case P_ROTHER:
                append_chars_str(str, entry->st.st_mode & S_IROTH ? "r" : "-");
                break;
            case P_WOTHER:
                append_chars_str(str, entry->st.st_mode & S_IWOTH ? "w" : "-");
                break;
            case P_XOTHER:
                append_chars_str(str, entry->st.st_mode & S_IXOTH ? "x" : "-");
                break;
            case P_ATTR:
                if (has_xattr_(entry->path->str, "system.posix_acl_access")) {
                    append_chars_str(str, "+");
                } else if (has_xattr_(entry->path->str, "security.selinux")) {
                    append_chars_str(str, ".");
                }
                break;
            default: ASSERT_FALSE(true);
        }
    }

    ASSERT_(str->len == UINT64_C(10) || str->len == UINT64_C(11),
            "unexpected permission length: %llu", (unsigned long long)str->len);
    ASSERT_EQ(str->pos, 0);
    return str;
failed:
    return NULL;
}

static t_str *get_user_(free_list *fl, uid_t user_id) {
    ASSERT_NOTNULL(fl);

    if (user_id == cached_uid) {
        return create_str(fl, cached_user);
    }

    const struct passwd *pwd = getpwuid(user_id);
    t_str *new_str = NULL;
    if (pwd) {
        new_str = create_str(fl, pwd->pw_name);
        if (!new_str) {
            return NULL;
        }

        ft_strlcpy(cached_user, pwd->pw_name, sizeof(cached_user));
    } else {
        new_str = uint_to_str(fl, user_id);
        if (!new_str) {
            return NULL;
        }

        ft_strlcpy(cached_user, new_str->str, sizeof(cached_user));
    }

    cached_uid = user_id;
    ASSERT_LE(new_str->len, LOGIN_NAME_MAX - 1);
    ASSERT_EQ(new_str->pos, 0);
    return new_str;
}

static t_str *get_group_(free_list *fl, gid_t group_id) {
    ASSERT_NOTNULL(fl);

    if (group_id == cached_gid) {
        return create_str(fl, cached_group);
    }

    const struct group *grp = getgrgid(group_id);
    t_str *new_str = NULL;
    if (grp) {
        new_str = create_str(fl, grp->gr_name);
        if (!new_str) {
            return NULL;
        }

        ft_strlcpy(cached_group, grp->gr_name, sizeof(cached_group));
    } else {
        new_str = uint_to_str(fl, group_id);
        if (!new_str) {
            return NULL;
        }

        ft_strlcpy(cached_group, new_str->str, sizeof(cached_group));
    }

    cached_gid = group_id;
    ASSERT_LE(new_str->len, LOGIN_NAME_MAX - 1);
    ASSERT_EQ(new_str->pos, 0);
    return new_str;
}

static t_str *get_dt_(free_list *fl, const struct timespec *ctim) {
    ASSERT_NOTNULL(fl);
    ASSERT_NOTNULL(ctim);

    const char *dt = ctime(&ctim->tv_sec);
    if (!dt) {
        return NULL;
    }

    const size_t len = ft_strlen(dt);
    if (len < 16) {
        return NULL;
    }

    t_str *new_str = init_str(fl, DT_LEN);
    if (!new_str) {
        return NULL;
    }

    ft_memcpy(new_str->str, dt + 4, 7);
    ft_memcpy(new_str->str + 7, dt + 11, 5);
    new_str->len = 12;
    new_str->str[new_str->len] = '\0';

    ASSERT_EQ(new_str->len, DT_LEN - 1);
    ASSERT_EQ(new_str->pos, 0);
    return new_str;
}

static bool get_symlink_(free_list *fl, const t_entry *entry, t_str **out) {
    ASSERT_NOTNULL(fl);
    ASSERT_NOTNULL(entry);
    ASSERT_NOTNULL(out);
    ASSERT_NOTNULL(*out);

    *out = NULL;
    if (!S_ISLNK(entry->st.st_mode)) {
        return true;
    }

    uint64_t cap = (entry->st.st_size > 0) ? (uint64_t)entry->st.st_size
                                           : (uint64_t)PATH_MAX;
    while (true) {
        t_str *new_str = init_str(fl, cap);
        if (!new_str) {
            break;
        }

        ssize_t len = readlink(entry->path->str, new_str->str, (size_t)cap);
        if (len < 0) {
            break;
        }

        if ((uint64_t)len < cap) {
            new_str->len = (uint64_t)len;
            new_str->str[new_str->len] = '\0';
            *out = new_str;
            return true;
        }

        if (cap > UINT64_MAX / 2) {
            break;
        }

        cap *= 2;
    }

    return false;
}

static bool has_xattr_(const char *path, const char *name) {
    ASSERT_NOTNULL(path);
    ASSERT_NOTNULL(name);
    ASSERT_(*path, "%c can not be '\\0'", *path);
    ASSERT_(*name, "%c can not be '\\0'", *name);

    ssize_t n = getxattr(path, name, NULL, 0);
    if (n < 0) {
        return false;
    }

    return true;
}

static t_str *join_paths_(free_list *fl, const t_str *lhs, const t_str *rhs) {
    ASSERT_NOTNULL(fl);
    ASSERT_NOTNULL(lhs);
    ASSERT_NOTNULL(rhs);

    const size_t new_len = lhs->len + 1 + rhs->len + 1;
    t_str *fullname = init_str(fl, new_len);
    if (!fullname) {
        return NULL;
    }

    char slash_buffer[] = "/";
    t_str slash = {.str = slash_buffer, .cap = 2, .len = 1, .pos = 0};
    (void)cat_str(fullname, lhs);
    if (fullname->str[fullname->len - 1] != '/') {
        (void)cat_str(fullname, &slash);
    }
    (void)cat_str(fullname, rhs);

    return fullname;
}
