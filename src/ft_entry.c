#include <grp.h>
#include <linux/limits.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <time.h>
#include <unistd.h>

#include "../include/ft_arena.h"
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
static void free_info_(free_list *fl, t_file_info *info);
static char get_attr_marker_(const char *path);
static void fill_perm_(const t_entry *entry, t_str *str);
static bool fill_dt_(t_str *new_str, const struct timespec *ctim);
static bool read_symlink_(const t_entry *entry, t_str *new_str, uint64_t cap);

static uid_t cached_uid = (uid_t)-1;
static gid_t cached_gid = (gid_t)-1;
static char cached_user[LOGIN_NAME_MAX] = "";
static char cached_group[LOGIN_NAME_MAX] = "";

t_entry *new_entry(Arena *arena, const t_entry *entry,
                   const struct dirent *dp) {
    t_shell_scan scan;
    Arena_Mark mark = arena_get_mark(arena);
    t_entry *ent = arena_push(arena, sizeof(*ent));
    if (!ent) {
        return NULL;
    }

    shell_scan_cstr(dp->d_name, &scan);
    ent->name = init_str_arena(arena, scan.len);
    if (!ent->name) {
        arena_pop_to_mark(arena, mark);
        return NULL;
    }

    ft_memcpy(ent->name->str, dp->d_name, scan.len);
    ent->name->len = scan.len;
    ent->name->str[ent->name->len] = '\0';
    ent->path = NULL;
    ent->parent_path = entry->path;
    ent->quote = scan.quote;
    ent->path_has_colon = entry->path_has_colon ||
                          ft_memchr(dp->d_name, ':', (size_t)scan.len) != NULL;
    ent->is_operand = false;
    ent->info = NULL;
    ent->st = (struct stat){0};
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
    Arena_Mark mark = arena_get_mark(arena);
    t_file_info *info = arena_push(arena, sizeof(*info));
    if (!info) {
        goto failed;
    }

    info->perm = get_perm_arena_(arena, entry);
    if (!info->perm) {
        goto failed;
    }

    info->links = uint_to_str_arena(arena, entry->st.st_nlink);
    if (!info->links) {
        goto failed;
    }

    info->username = get_user_arena_(arena, entry->st.st_uid);
    if (!info->username) {
        goto failed;
    }

    info->groupname = get_group_arena_(arena, entry->st.st_gid);
    if (!info->groupname) {
        goto failed;
    }

    info->size = uint_to_str_arena(arena, (uint64_t)entry->st.st_size);
    if (!info->size) {
        goto failed;
    }

    info->dt = get_dt_arena_(arena, &entry->st.st_mtim);
    if (!info->dt) {
        goto failed;
    }

    info->symlink = get_symlink_arena_(arena, entry);
    if (!info->symlink) {
        goto failed;
    }

    info->blocks = (uint64_t)entry->st.st_blocks;
    entry->info = info;
    return true;
failed:
    arena_pop_to_mark(arena, mark);
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

    fill_perm_(entry, str);
    return str;
}

static t_str *get_perm_arena_(Arena *arena, const t_entry *entry) {
    t_str *str = init_str_arena(arena, PERMISSION_SIZE);
    if (!str) {
        return NULL;
    }

    fill_perm_(entry, str);
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
    t_str *new_str = init_str(fl, DT_LEN);
    if (!new_str) {
        return NULL;
    }

    if (!fill_dt_(new_str, ctim)) {
        fl_free(fl, new_str);
        return NULL;
    }

    return new_str;
}

static t_str *get_dt_arena_(Arena *arena, const struct timespec *ctim) {
    Arena_Mark mark = arena_get_mark(arena);
    t_str *new_str = init_str_arena(arena, DT_LEN);
    if (!new_str) {
        return NULL;
    }

    if (!fill_dt_(new_str, ctim)) {
        arena_pop_to_mark(arena, mark);
        return NULL;
    }
    return new_str;
}

static t_str *get_symlink_(free_list *fl, const t_entry *entry) {
    if (!S_ISLNK(entry->st.st_mode)) {
        return init_str(fl, 1);
    }

    uint64_t cap = (entry->st.st_size > 0) ? (uint64_t)entry->st.st_size + 1
                                           : (uint64_t)PATH_MAX;
    while (true) {
        t_str *new_str = init_str(fl, cap);
        if (!new_str) {
            break;
        }

        if (!read_symlink_(entry, new_str, cap)) {
            return new_str;
        }

        free_str(fl, new_str);
        if (cap > UINT64_MAX / 2) {
            return NULL;
        }

        cap *= 2;
    }

    return NULL;
}

static t_str *get_symlink_arena_(Arena *arena, const t_entry *entry) {
    if (!S_ISLNK(entry->st.st_mode)) {
        return init_str_arena(arena, 1);
    }

    Arena_Mark mark = arena_get_mark(arena);
    uint64_t cap = (entry->st.st_size > 0) ? (uint64_t)entry->st.st_size + 1
                                           : (uint64_t)PATH_MAX;
    while (true) {
        t_str *new_str = init_str_arena(arena, cap);
        if (!new_str) {
            return NULL;
        }

        if (!read_symlink_(entry, new_str, cap)) {
            return new_str;
        }

        arena_pop_to_mark(arena, mark);
        if (cap > UINT64_MAX / 2) {
            return NULL;
        }

        cap *= 2;
    }
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

static char get_attr_marker_(const char *path) {
    ssize_t len = listxattr(path, NULL, 0);
    if (len <= 0) {
        return '\0';
    }

    char *buf = malloc((size_t)len);
    if (!buf) {
        return '\0';
    }

    len = listxattr(path, buf, (size_t)len);
    if (len <= 0) {
        free(buf);
        return '\0';
    }

    bool has_selinux = false;
    const char *end = buf + len;
    for (char *name = buf; name < end; name += strlen(name) + 1) {
        if (strcmp(name, "system.posix_acl_access") == 0) {
            free(buf);
            return '+';
        }
        if (strcmp(name, "security.selinux") == 0) {
            has_selinux = true;
        }
    }

    free(buf);
    return has_selinux ? '.' : '\0';
}

static void fill_perm_(const t_entry *entry, t_str *str) {
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

    char marker = get_attr_marker_(entry->path->str);
    if (marker == '+' || marker == '.') {
        str->str[index++] = marker;
    }

    str->len = index;
    str->str[index] = '\0';
}

static bool fill_dt_(t_str *new_str, const struct timespec *ctim) {
    const char *dt = ctime(&ctim->tv_sec);
    if (!dt) {
        return false;
    }

    const size_t len = ft_strlen(dt);
    if (len < 16) {
        return false;
    }

    ft_memcpy(new_str->str, dt + 4, 7);
    ft_memcpy(new_str->str + 7, dt + 11, 5);
    new_str->len = 12;
    new_str->str[new_str->len] = '\0';
    return true;
}

static bool read_symlink_(const t_entry *entry, t_str *new_str, uint64_t cap) {
    ssize_t len = readlink(entry->path->str, new_str->str, (size_t)cap);
    if (len < 0) {
        new_str->len = 0;
        new_str->str[0] = '\0';
        return false;
    }

    if ((uint64_t)len < cap) {
        new_str->len = (uint64_t)len;
        new_str->str[new_str->len] = '\0';
        return false;
    }

    return true;
}
