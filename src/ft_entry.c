#include <grp.h>
#include <linux/limits.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <time.h>

#include "../include/ft_entry.h"
#include "../include/ft_free_list.h"
#include "../include/ft_shell_escape.h"
#include "../include/ft_str.h"

#include "../libft/include/libft.h"

static t_str *get_perm_(free_list *fl, const t_entry *entry);
static t_str *get_perm_arena_(Arena *arena, const t_entry *entry);
static t_str *get_user_(free_list *fl, uid_t user_id);
static t_str *get_user_arena_(Arena *arena, uid_t user_id);
static t_str *get_group_(free_list *fl, gid_t group_id);
static t_str *get_group_arena_(Arena *arena, gid_t group_id);
static t_str *get_dt_(free_list *fl, const struct timespec *ctim);
static t_str *get_dt_arena_(Arena *arena, const struct timespec *ctim);
static t_str *get_symlink_(free_list *fl, const t_entry *entry);
static t_str *get_symlink_arena_(Arena *arena, const t_entry *entry);
static bool has_xattr_(const char *path, const char *name);
static t_str *join_paths_(free_list *fl, const t_str *lhs, const t_str *rhs);
static t_str *join_paths_arena_(Arena *arena, const t_str *lhs,
                                const t_str *rhs);
static void free_info_(free_list *fl, t_file_info *info);
static bool is_alpha_ascii_(unsigned char c);
static bool is_digit_ascii_(unsigned char c);
static bool is_print_ascii_(unsigned char c);
static bool is_safe_punct_(unsigned char c, uint64_t index);
static bool needs_raw_quote_char_(unsigned char c, uint64_t index);
static uint64_t ansi_byte_len_(unsigned char byte);

typedef struct {
    uint64_t len;
    uint64_t display_len;
    uint64_t padded_display_len;
    char quote;
} t_name_scan;

static void scan_name_(const char *src, t_name_scan *scan);

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
    t_entry *ent = fl_alloc(fl, sizeof(*ent), 8);
    if (!ent) {
        goto failed;
    }

    *ent = (t_entry){0};

    ent->name = create_str(fl, dp->d_name);
    if (!ent->name) {
        goto failed;
    }

    ent->path = join_paths_(fl, entry->path, ent->name);
    if (!ent->path) {
        goto failed;
    }

    ent->quote = shell_quote(ent->name);
    ent->is_escaped = false;
    ent->is_operand = false;
    ent->info = NULL;
    init_entry_display(ent);

    return ent;
failed:
    if (ent) {
        free_entry(fl, ent);
    }

    return NULL;
}

t_entry *new_entry_arena(Arena *arena, t_entry *entry,
                         const struct dirent *dp) {
    t_name_scan scan;
    t_entry *ent = arena_push(arena, sizeof(*ent));
    if (!ent) {
        return NULL;
    }

    scan_name_(dp->d_name, &scan);
    ent->name = init_str_arena(arena, scan.len);
    if (!ent->name) {
        return NULL;
    }

    ft_memcpy(ent->name->str, dp->d_name, scan.len);
    ent->name->len = scan.len;
    ent->name->str[ent->name->len] = '\0';

    ent->path = join_paths_arena_(arena, entry->path, ent->name);
    if (!ent->path) {
        return NULL;
    }

    ent->quote = scan.quote;
    ent->is_escaped = false;
    ent->is_operand = false;
    ent->info = NULL;
    ent->display_len = scan.display_len;
    ent->padded_display_len = scan.padded_display_len;
    return ent;
}

bool get_file_info(free_list *fl, t_entry *entry) {
    t_file_info *info = fl_alloc(fl, sizeof(*info), 8);
    if (!info) {
        goto failed;
    }

    *info = (t_file_info){0};

    info->perm = get_perm_(fl, entry);
    if (!info->perm) {
        goto failed;
    }

    info->links = uint_to_str(fl, entry->st.st_nlink);
    if (!info->links) {
        goto failed;
    }

    info->username = get_user_(fl, entry->st.st_uid);
    if (!info->username) {
        goto failed;
    }

    info->groupname = get_group_(fl, entry->st.st_gid);
    if (!info->groupname) {
        goto failed;
    }

    info->size = uint_to_str(fl, (uint64_t)entry->st.st_size);
    if (!info->size) {
        goto failed;
    }

    info->dt = get_dt_(fl, &entry->st.st_mtim);
    if (!info->dt) {
        goto failed;
    }

    info->symlink = get_symlink_(fl, entry);
    if (!info->symlink) {
        goto failed;
    }

    info->blocks = (uint64_t)entry->st.st_blocks;

    entry->info = info;
    return true;
failed:
    if (info) {
        free_info_(fl, info);
    }

    entry->info = NULL;
    return false;
}

bool get_file_info_arena(Arena *arena, t_entry *entry) {
    t_file_info *info = arena_push(arena, sizeof(*info));
    if (!info) {
        return false;
    }

    info->perm = get_perm_arena_(arena, entry);
    if (!info->perm) {
        return false;
    }

    info->links = uint_to_str_arena(arena, entry->st.st_nlink);
    if (!info->links) {
        return false;
    }

    info->username = get_user_arena_(arena, entry->st.st_uid);
    if (!info->username) {
        return false;
    }

    info->groupname = get_group_arena_(arena, entry->st.st_gid);
    if (!info->groupname) {
        return false;
    }

    info->size = uint_to_str_arena(arena, (uint64_t)entry->st.st_size);
    if (!info->size) {
        return false;
    }

    info->dt = get_dt_arena_(arena, &entry->st.st_mtim);
    if (!info->dt) {
        return false;
    }

    info->symlink = get_symlink_arena_(arena, entry);
    if (!info->symlink) {
        return false;
    }

    info->blocks = (uint64_t)entry->st.st_blocks;
    entry->info = info;
    return true;
}

void init_entry_display(t_entry *entry) {
    entry->display_len = shell_display_len(entry->name, entry->quote, false);
    if (entry->quote == '\0') {
        entry->padded_display_len = entry->display_len + 1;
    } else {
        entry->padded_display_len = entry->display_len;
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

    if (entry->info) {
        free_info_(fl, entry->info);
    }

    fl_free(fl, entry);
}

static t_str *get_perm_(free_list *fl, const t_entry *entry) {
    t_str *str = init_str(fl, PERMISSION_SIZE);
    if (!str) {
        return NULL;
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
            default: free_str(fl, str); return NULL;
        }
    }

    return str;
}

static t_str *get_perm_arena_(Arena *arena, const t_entry *entry) {
    t_str *str = init_str_arena(arena, PERMISSION_SIZE);
    if (!str) {
        return NULL;
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
            default: return NULL;
        }
    }

    return str;
}

static t_str *get_user_(free_list *fl, uid_t user_id) {
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
    return new_str;
}

static t_str *get_user_arena_(Arena *arena, uid_t user_id) {
    if (user_id == cached_uid) {
        return create_str_arena(arena, cached_user);
    }

    const struct passwd *pwd = getpwuid(user_id);
    t_str *new_str = NULL;
    if (pwd) {
        new_str = create_str_arena(arena, pwd->pw_name);
        if (!new_str) {
            return NULL;
        }

        ft_strlcpy(cached_user, pwd->pw_name, sizeof(cached_user));
    } else {
        new_str = uint_to_str_arena(arena, user_id);
        if (!new_str) {
            return NULL;
        }

        ft_strlcpy(cached_user, new_str->str, sizeof(cached_user));
    }

    cached_uid = user_id;
    return new_str;
}

static t_str *get_group_(free_list *fl, gid_t group_id) {
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
    return new_str;
}

static t_str *get_group_arena_(Arena *arena, gid_t group_id) {
    if (group_id == cached_gid) {
        return create_str_arena(arena, cached_group);
    }

    const struct group *grp = getgrgid(group_id);
    t_str *new_str = NULL;
    if (grp) {
        new_str = create_str_arena(arena, grp->gr_name);
        if (!new_str) {
            return NULL;
        }

        ft_strlcpy(cached_group, grp->gr_name, sizeof(cached_group));
    } else {
        new_str = uint_to_str_arena(arena, group_id);
        if (!new_str) {
            return NULL;
        }

        ft_strlcpy(cached_group, new_str->str, sizeof(cached_group));
    }

    cached_gid = group_id;
    return new_str;
}

static t_str *get_dt_(free_list *fl, const struct timespec *ctim) {
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

    return new_str;
}

static t_str *get_dt_arena_(Arena *arena, const struct timespec *ctim) {
    const char *dt = ctime(&ctim->tv_sec);
    if (!dt) {
        return NULL;
    }

    const size_t len = ft_strlen(dt);
    if (len < 16) {
        return NULL;
    }

    t_str *new_str = init_str_arena(arena, DT_LEN);
    if (!new_str) {
        return NULL;
    }

    ft_memcpy(new_str->str, dt + 4, 7);
    ft_memcpy(new_str->str + 7, dt + 11, 5);
    new_str->len = 12;
    new_str->str[new_str->len] = '\0';
    return new_str;
}

static t_str *get_symlink_(free_list *fl, const t_entry *entry) {
    if (!S_ISLNK(entry->st.st_mode)) {
        return init_str(fl, 1);
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
            free_str(fl, new_str);
            break;
        }

        if ((uint64_t)len < cap) {
            new_str->len = (uint64_t)len;
            new_str->str[new_str->len] = '\0';
            return new_str;
        }

        if (cap > UINT64_MAX / 2) {
            free_str(fl, new_str);
            break;
        }

        free_str(fl, new_str);
        cap *= 2;
    }

    return NULL;
}

static t_str *get_symlink_arena_(Arena *arena, const t_entry *entry) {
    if (!S_ISLNK(entry->st.st_mode)) {
        return init_str_arena(arena, 1);
    }

    uint64_t cap = (entry->st.st_size > 0) ? (uint64_t)entry->st.st_size
                                           : (uint64_t)PATH_MAX;
    while (true) {
        t_str *new_str = init_str_arena(arena, cap);
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
            return new_str;
        }

        if (cap > UINT64_MAX / 2) {
            break;
        }

        cap *= 2;
    }

    return NULL;
}

static bool has_xattr_(const char *path, const char *name) {
    ssize_t n = getxattr(path, name, NULL, 0);
    if (n < 0) {
        return false;
    }

    return true;
}

static t_str *join_paths_(free_list *fl, const t_str *lhs, const t_str *rhs) {
    const bool need_slash = lhs->len == 0 || lhs->str[lhs->len - 1] != '/';
    const size_t new_len = lhs->len + rhs->len + (need_slash ? 1U : 0U) + 1U;
    t_str *fullname = init_str(fl, new_len);
    if (!fullname) {
        return NULL;
    }

    ft_memcpy(fullname->str, lhs->str, lhs->len);
    fullname->len = lhs->len;
    if (need_slash) {
        fullname->str[fullname->len++] = '/';
    }
    ft_memcpy(fullname->str + fullname->len, rhs->str, rhs->len);
    fullname->len += rhs->len;
    fullname->str[fullname->len] = '\0';

    return fullname;
}

static t_str *join_paths_arena_(Arena *arena, const t_str *lhs,
                                const t_str *rhs) {
    const bool need_slash = lhs->len == 0 || lhs->str[lhs->len - 1] != '/';
    const size_t new_len = lhs->len + rhs->len + (need_slash ? 1U : 0U) + 1U;
    t_str *fullname = init_str_arena(arena, new_len);
    if (!fullname) {
        return NULL;
    }

    ft_memcpy(fullname->str, lhs->str, lhs->len);
    fullname->len = lhs->len;
    if (need_slash) {
        fullname->str[fullname->len++] = '/';
    }
    ft_memcpy(fullname->str + fullname->len, rhs->str, rhs->len);
    fullname->len += rhs->len;
    fullname->str[fullname->len] = '\0';
    return fullname;
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

static bool is_alpha_ascii_(unsigned char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static bool is_digit_ascii_(unsigned char c) {
    return c >= '0' && c <= '9';
}

static bool is_print_ascii_(unsigned char c) {
    return c >= ' ' && c <= '~';
}

static bool is_safe_punct_(unsigned char c, uint64_t index) {
    switch (c) {
        case '_':
        case '-':
        case '.':
        case '/':
        case '+':
        case ':':
        case ',':
        case '@':
        case '%':
        case '{':
        case '}': return true;
        case '#':
        case '~': return index != 0;
        default: return false;
    }
}

static bool needs_raw_quote_char_(unsigned char c, uint64_t index) {
    switch (c) {
        case ' ':
        case '\'':
        case '"':
        case '!':
        case '&':
        case '(':
        case ')':
        case '[':
        case ']':
        case '*':
        case '?':
        case '<':
        case '>':
        case '|':
        case ';':
        case '$':
        case '`':
        case '\\':
        case '^':
        case '=': return true;
        default:
            return !is_alpha_ascii_(c) && !is_digit_ascii_(c) &&
                   !is_safe_punct_(c, index);
    }
}

static uint64_t ansi_byte_len_(unsigned char byte) {
    switch (byte) {
        case '\a':
        case '\b':
        case '\t':
        case '\n':
        case '\v':
        case '\f':
        case '\r': return 2;
        default: return 4;
    }
}

static void scan_name_(const char *src, t_name_scan *scan) {
    bool needs_quote = false;
    bool has_single = false;
    bool can_use_double = true;
    bool in_single = true;
    bool in_ansi = false;
    bool has_non_print = false;
    uint64_t escaped_len = 1;

    scan->len = 0;
    while (src[scan->len] != '\0') {
        const unsigned char c = (unsigned char)src[scan->len];
        if (!is_print_ascii_(c)) {
            has_non_print = true;
        }

        if (needs_raw_quote_char_(c, scan->len)) {
            needs_quote = true;
            if (c != ' ' && c != '\'') {
                can_use_double = false;
            }
        }

        if (c == '\'') {
            has_single = true;
        }

        if (is_print_ascii_(c) && c != '\'') {
            if (in_ansi) {
                ++escaped_len;
                in_ansi = false;
            }
            if (!in_single) {
                ++escaped_len;
                in_single = true;
            }
            ++escaped_len;
        } else if (c == '\'') {
            if (in_ansi) {
                ++escaped_len;
                in_ansi = false;
            }
            escaped_len += 4;
            in_single = true;
        } else {
            if (in_single) {
                ++escaped_len;
                in_single = false;
            }
            if (!in_ansi) {
                escaped_len += 2;
                in_ansi = true;
            }
            escaped_len += ansi_byte_len_(c);
        }

        ++scan->len;
    }

    if (in_ansi || in_single) {
        ++escaped_len;
    }

    if (has_non_print) {
        scan->quote = '\'';
    } else if (!needs_quote) {
        scan->quote = '\0';
    } else if (has_single && can_use_double) {
        scan->quote = '"';
    } else {
        scan->quote = '\'';
    }

    if (scan->quote == '\0') {
        scan->display_len = scan->len;
        scan->padded_display_len = scan->len + 1;
    } else if (scan->quote == '"') {
        scan->display_len = scan->len + 2;
        scan->padded_display_len = scan->display_len;
    } else {
        scan->display_len = escaped_len;
        scan->padded_display_len = escaped_len;
    }
}
