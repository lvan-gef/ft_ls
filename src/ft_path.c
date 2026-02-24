#include <stdint.h>
#include <sys/stat.h>

#include "../include/ft_arena.h"
#include "../include/ft_assert.h"
#include "../include/ft_path.h"
#include "../include/ft_str.h"

#include "../libft/include/libft.h"
#include "ft_fprintf.h"

static t_str *get_perm_(Arena *arena, t_entry *entry);

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
    P_TOTAL
} t_perm_lttr;

// static uid_t cached_uid = (uid_t)-1;
// static gid_t cached_gid = (gid_t)-1;
// static char cached_user[LOGIN_NAME_MAX] = "";
// static char cached_group[LOGIN_NAME_MAX] = "";

t_str *get_path_entry(Arena *arena, t_entry *entry) {
    ASSERT_NOTNULL(entry);
    ASSERT_NOTNULL(entry->path);
    ASSERT_GT(entry->path->cap, 0);
    ASSERT_LT(entry->path->len, entry->path->cap);
    ASSERT_NOTNULL(arena);

    const uint64_t arena_pos = ArenaPos(arena);
    char *str = ArenaPush(arena, entry->path->len);
    if (!str) {
        goto failed;
    }
    ft_strlcpy(str, entry->path->str, entry->path->cap);

    char *last_slash = ft_strrchr(str, '/');
    if (!last_slash) {
        goto failed;
    }

    long len = last_slash - str;
    str[len] = '\0';

    t_str *new_str = create_str(arena, str);
    if (!new_str) {
        goto failed;
    }

    return new_str;

failed:
    if (str) {
        ArenaPopTo(arena, arena_pos);
    }

    return NULL;
}

bool get_file_info(Arena *arena, t_entry *entry) {
    ASSERT_NOTNULL(entry);
    ASSERT_NOTNULL(entry->path);
    ASSERT_GT(entry->path->cap, 0);
    ASSERT_LT(entry->path->len, entry->path->cap);
    ASSERT_NOTNULL(arena);

    const uint64_t arena_pos = ArenaPos(arena);
    t_file_info *info = ArenaPush(arena, sizeof(*info));
    if (!info) {
        goto failed;
    }

    info->perm = get_perm_(arena, entry);
    if (!info->perm) {
        goto failed;
    }

    info->links = uint_to_str(arena, entry->st.st_nlink);
    if (!info->links) {
        goto failed;
    }

    ft_fprintf(STDOUT_FILENO, "%s %s\n", info->perm->str, info->links->str);

    entry->info = info;
    return true;

failed:
    if (info) {
        ArenaPopTo(arena, arena_pos);
    }

    return false;
}

static t_str *get_perm_(Arena *arena, t_entry *entry) {
    const uint64_t arena_pos = ArenaPos(arena);

    t_str *str = init_str(arena, PERMISSION_SIZE);
    if (!str) {
        goto failed;
    }

    Arena *scratch = ArenaAlloc(ARENA_SIZE);
    if (!scratch) {
        goto failed;
    }
    ArenaSetAutoAlign(arena, 8);

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
            default: ASSERT_FALSE(true);
        }
    }

    ArenaRelease(scratch);
    return str;
failed:
    if (str) {
        ArenaPopTo(arena, arena_pos);
    }

    return NULL;
}
