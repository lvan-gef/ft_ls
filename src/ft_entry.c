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

#include "../libft/include/libft.h"

#ifndef CACHE_SIZE
#define CACHE_SIZE UINT64_C(8)
#endif

typedef struct s_user {
    char name[LOGIN_NAME_MAX];
    uid_t user_id;
} t_user;

typedef struct s_group {
    char name[LOGIN_NAME_MAX];
    gid_t group_id;
} t_group;

static uint64_t user_index = 0;
static t_user user_cache[CACHE_SIZE] = {0};

static uint64_t group_index = 0;
static t_group group_cache[CACHE_SIZE] = {0};

static bool fill_file_info_(const t_alloc *alloc, t_file_info *info,
                            const t_entry *entry);
static t_str *get_perm_(const t_alloc *alloc, const t_entry *entry);
static t_str *get_user_(const t_alloc *alloc, uid_t user_id);
static t_str *get_group_(const t_alloc *alloc, gid_t group_id);
static t_str *get_dt_(const t_alloc *alloc, const struct timespec *ctim);
static t_str *get_symlink_(const t_alloc *alloc, const t_entry *entry);
static void free_info_(free_list *fl, t_file_info *info);
static void free_info_cb_(free_list *fl, void *ptr);
static void free_str_cb_(free_list *fl, void *ptr);
static char *user_cached(uid_t user_id);
static void add_user_cache_(uid_t user_id, const char *username);
static char *group_cached(gid_t group_id);
static void add_group_cache_(gid_t group_id, const char *groupname);

t_entry *new_entry(const t_alloc *alloc, const t_entry *entry,
                   const struct dirent *dp) {
    Arena_Mark mark = {0};
    t_entry *ent = NULL;

    switch (alloc->kind) {
        case ALLOC_ARENA:
            mark = arena_get_mark(alloc->as.arena);
            ent = arena_push(alloc->as.arena, sizeof(*ent));
            break;
        case ALLOC_FL: ent = fl_alloc(alloc->as.fl, sizeof(*ent), 8); break;
    }

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
    t_file_info *info = NULL;
    Arena_Mark mark = {0};
    switch (alloc->kind) {
        case ALLOC_ARENA:
            mark = arena_get_mark(alloc->as.arena);
            info = arena_push(alloc->as.arena, sizeof(*info));
            break;
        case ALLOC_FL: info = fl_alloc(alloc->as.fl, sizeof(*info), 8); break;
    }

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
    const char *user_name = user_cached(user_id);
    if (user_name) {
        return create_str(alloc, user_name);
    }

    const struct passwd *pwd = getpwuid(user_id);
    t_str *new_str = NULL;
    if (pwd) {
        new_str = create_str(alloc, pwd->pw_name);
        if (!new_str) {
            return NULL;
        }
        add_user_cache_(user_id, pwd->pw_name);
    } else {
        const int err = errno;

        new_str = uint_to_str(alloc, (uint64_t)user_id);
        if (!new_str) {
            return NULL;
        }

        if (!err) {
            add_user_cache_(user_id, new_str->str);
        }
    }

    return new_str;
}

static t_str *get_group_(const t_alloc *alloc, gid_t group_id) {
    const char *group_name = group_cached(group_id);
    if (group_name) {
        return create_str(alloc, group_name);
    }

    const struct group *grp = getgrgid(group_id);
    t_str *new_str = NULL;
    if (grp) {
        new_str = create_str(alloc, grp->gr_name);
        if (!new_str) {
            return NULL;
        }

        add_group_cache_(group_id, grp->gr_name);
    } else {
        const int err = errno;

        new_str = uint_to_str(alloc, (uint64_t)group_id);
        if (!new_str) {
            return NULL;
        }

        if (!err) {
            add_group_cache_(group_id, new_str->str);
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

static void free_str_cb_(free_list *fl, void *ptr) {
    free_str(fl, ptr);
}

static char *user_cached(uid_t user_id) {
    for (uint64_t index = 0; index < CACHE_SIZE; ++index) {
        if (!*user_cache[index].name) {
            continue;
        }

        if (user_id == user_cache[index].user_id) {
            return user_cache[index].name;
        }
    }

    return NULL;
}

static void add_user_cache_(uid_t user_id, const char *username) {
    if (!username || !*username) {
        return;
    }

    const uint64_t index = user_index % CACHE_SIZE;
    user_cache[index].user_id = user_id;
    ft_strlcpy(user_cache[index].name, username, LOGIN_NAME_MAX);
    ++user_index;
}

static char *group_cached(gid_t group_id) {
    for (uint64_t index = 0; index < CACHE_SIZE; ++index) {
        if (!*group_cache[index].name) {
            continue;
            ;
        }

        if (group_id == group_cache[index].group_id) {
            return group_cache[index].name;
        }
    }

    return NULL;
}

static void add_group_cache_(gid_t group_id, const char *groupname) {
    if (!groupname || !*groupname) {
        return;
    }

    const uint64_t index = group_index % CACHE_SIZE;
    group_cache[index].group_id = group_id;
    ft_strlcpy(group_cache[index].name, groupname, LOGIN_NAME_MAX);
    ++group_index;
}
