
#include <errno.h>
#include <grp.h>
#include <linux/limits.h>
#include <pwd.h>
#include <string.h>
#include <sys/stat.h>

#include "../include/ft_get_stats.h"
#include "../include/ft_ls.h"
#include "../libft/include/ft_fprintf.h"
#include "../include/ft_arena.h"
#include "../include/ft_helpers.h"

#include "../libft/include/libft.h"

static uid_t cached_uid = (uid_t)-1;
static gid_t cached_gid = (gid_t)-1;
static char cached_user[USER_SIZE] = "";
static char cached_group[USER_SIZE] = "";

bool get_stat(struct stat *sb, const char *fullpath) {
    if (lstat(fullpath, sb) == -1) {
        ft_fprintf(STDERR_FILENO, "ft_ls: cannot access: '%s': %s\n", fullpath,
                   strerror(errno));
        return false;
    }

    return true;
}

bool get_permission(Arena *arena, struct stat *sb, t_file *file) {
    char permission[PERMISSION_SIZE] = {0};

    size_t len = 0;
    switch (sb->st_mode & S_IFMT) {
        case S_IFLNK:
            len += ft_strlcpy(permission + len, "l", PERMISSION_SIZE);
            break;
        case S_IFREG:
            len += ft_strlcpy(permission + len, "-", PERMISSION_SIZE);
            break;
        case S_IFDIR:
            len += ft_strlcpy(permission + len, "d", PERMISSION_SIZE);
            break;
        default:
            break;
    }

    len += ft_strlcpy(permission + len, (sb->st_mode & S_IRUSR) ? "r" : "-",
                      PERMISSION_SIZE);
    len += ft_strlcpy(permission + len, (sb->st_mode & S_IWUSR) ? "w" : "-",
                      PERMISSION_SIZE);
    len += ft_strlcpy(permission + len, (sb->st_mode & S_IXUSR) ? "x" : "-",
                      PERMISSION_SIZE);
    len += ft_strlcpy(permission + len, (sb->st_mode & S_IRGRP) ? "r" : "-",
                      PERMISSION_SIZE);
    len += ft_strlcpy(permission + len, (sb->st_mode & S_IWGRP) ? "w" : "-",
                      PERMISSION_SIZE);
    len += ft_strlcpy(permission + len, (sb->st_mode & S_IXGRP) ? "x" : "-",
                      PERMISSION_SIZE);
    len += ft_strlcpy(permission + len, (sb->st_mode & S_IROTH) ? "r" : "-",
                      PERMISSION_SIZE);
    len += ft_strlcpy(permission + len, (sb->st_mode & S_IWOTH) ? "w" : "-",
                      PERMISSION_SIZE);
    len += ft_strlcpy(permission + len, (sb->st_mode & S_IXOTH) ? "x" : "-",
                      PERMISSION_SIZE);

    file->permission = create_str(arena, permission);
    if (!file->permission) {
        return false;
    }

    return true;
}

bool get_hardlink(Arena *arena, struct stat *sb, t_file *file) {
    const size_t hardlink_len = get_len(sb->st_nlink) + 1;

    char *hardlink_str = ArenaPush(arena, hardlink_len * sizeof(char));
    if (!hardlink_str) {
        return false;
    }

    uitoa(hardlink_str, hardlink_len, sb->st_nlink);

    file->hardlink = ArenaPush(arena, sizeof(*file->hardlink));
    if (!file->hardlink) {
        return false;
    }

    file->hardlink->count = sb->st_nlink;
    file->hardlink->str = create_str(arena, hardlink_str);

    return true;
}

bool get_user(Arena *arena, t_file *file, uid_t user_id) {
    char id[USER_SIZE] = {0};

    if (user_id != cached_uid) {
        const struct passwd *pwd = getpwuid(user_id);
        if (pwd) {
            ft_strlcpy(cached_user, pwd->pw_name, sizeof(cached_user));
        } else {
            uitoa(id, USER_SIZE, user_id);
            ft_strlcpy(cached_user, id, sizeof(cached_user));
        }
        cached_uid = user_id;
    }

    file->user = create_str(arena, cached_user);
    if (!file->user) {
        return false;
    }

    return true;
}

bool get_group(Arena *arena, t_file *file, gid_t group_id) {
    char id[USER_SIZE] = {0};

    if (group_id != cached_gid) {
        const struct group *grp = getgrgid(group_id);
        if (grp) {
            ft_strlcpy(cached_group, grp->gr_name, sizeof(cached_group));
        } else {
            uitoa(id, USER_SIZE, group_id);
            ft_strlcpy(cached_group, id, sizeof(cached_group));
        }
        cached_gid = group_id;
    }

    file->group = create_str(arena, cached_group);
    if (!file->group) {
        return false;
    }

    return true;
}

bool get_size(Arena *arena, struct stat *sb, t_file *file) {
    const size_t size_len = get_len((size_t)sb->st_size) + 1;

    char *size_str = ArenaPush(arena, size_len * sizeof(char));
    if (!size_str) {
        return false;
    }

    uitoa(size_str, size_len, (size_t)sb->st_size);

    file->size = ArenaPush(arena, sizeof(*file->size));
    if (!file->size) {
        return false;
    }

    file->size->size = (size_t)sb->st_size;
    file->size->str = create_str(arena, size_str);

    return true;
}

bool get_dt(Arena *arena, struct stat *sb, t_file *file) {
#if defined(__linux__)
    char *dt = ctime(&sb->st_mtim.tv_sec);
#elif defined(__APPLE__)
    char *dt = ctime(&sb->st_mtimespec.tv_sec);
#else
    ft_fprintf(STDERR_FILENO, "OS is not supported\n");
    return false;
#endif
    if (!dt) {
        return false;
    }
    char buffer[DT_LEN] = {0};


    // TODO: dont want to use malloc
    char **splitter = ft_split(dt, ' ');
    if (!splitter) {
        // TODO: print error
        return false;
    }

    size_t len = ft_strlcpy(buffer, splitter[2], DT_LEN);
    len += ft_strlcpy(buffer + len, " ", DT_LEN);
    len += ft_strlcpy(buffer + len, splitter[1], DT_LEN);
    len += ft_strlcpy(buffer + len, " ", DT_LEN);
    len += ft_strlcpy(buffer + len, splitter[3], DT_LEN);
    buffer[len - 3] = '\0'; // remove seconds
    ft_str_to_lower(buffer);

    size_t index = 0;
    while (splitter[index]) {
        free(splitter[index]);
        ++index;
    }
    free((void *)splitter);

    file->dt = create_str(arena, buffer);
    if (!file->dt) {
        return false;
    }

    return true;
}

bool get_linked_name(Arena *arena, struct stat *sb, t_file *file, const char *fullname) {
    char filename[NAME_MAX] = {0};

    if (S_ISLNK(sb->st_mode)) {
        ssize_t len = readlink(filename, (char *)fullname, NAME_MAX - 1);
        if (len < 0) {
            return false;
        }
    }

    file->linked_name = create_str(arena, filename);
    if (!file->linked_name) {
        return false;
    }

    return true;
}
