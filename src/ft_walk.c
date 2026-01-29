#include <dirent.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#if defined(__linux__)
#include <linux/limits.h>
#elif defined(__APPLE__)
#include <sys/syslimits.h>
#endif

#include "../include/ft_arena.h"
#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_helpers.h"
#include "../include/ft_ls.h"

#include "../include/ft_walk.h"
#include "../libft/include/ft_fprintf.h"
#include "../libft/include/libft.h"

static char *walk_files_(Arena *arena, t_args *args, DIR *dir, t_path *path);
static char *parse_file_(Arena *arena, const struct dirent *dirent,
                         struct stat *sb, t_path *path, t_array *paths, const char *fullpath);
static bool create_path_node_(t_args *args, const t_path *path,
                              const char *pathname);
static void set_fullpath_(char *fullpath, const char *filename,
                          const char *dir_name);
static void get_user_group_(Arena *arena, t_file *file, unsigned int group_id,
                            unsigned int user_id);
static void get_permission_(t_file *file, const struct stat *sb);
static bool get_dt_(char *buffer, const struct stat *sb);
static struct timespec get_time_spec_(const struct stat *sb);
static void set_filename(t_file *file, const char *filename, t_path *path);
static bool get_symlink_(char *buffer, const char *filename, struct stat *sb);

static uid_t cached_uid = (uid_t)-1;
static gid_t cached_gid = (gid_t)-1;
static char cached_user[USER_SIZE] = "";
static char cached_group[USER_SIZE] = "";

bool walk(t_args *args) {
    ASSERT_(args, "args can not be NULL");
    ASSERT_(args->paths, "args->paths can not be NULL");

    DIR *dir = NULL;
    char *err_msg = NULL;
    size_t index = 0;
    Arena *arena = ArenaAlloc((USER_SIZE * 2) + 1);
    if (!arena) {
        // TODO: print error
        return false;
    }

    while (index < args->paths->len) {
        errno = 0;
        t_path *path = args->paths->data[index];
        dir = opendir(path->name);
        if (!dir) {
            if (errno == EACCES) {
                ft_fprintf(STDERR_FILENO, "ft_ls: cannot access: '%s': %s\n",
                           path->name, strerror(errno));
                errno = 0;
                ++index;
                continue;
            }

            if (errno == ENOENT) {
                errno = 0;
                ++index;
                continue;
            }

            err_msg = strerror(errno);
            goto failed;
        }

        err_msg = walk_files_(arena, args, dir, path);
        if (err_msg || errno) {
            goto failed;
        }

        closedir(dir);
        ++index;
    }

    ArenaRelease(arena);
    return true;
failed:
    ft_fprintf(STDERR_FILENO, "errno: %d, %s\n", errno, err_msg);
    if (dir) {
        closedir(dir);
    }

    ArenaRelease(arena);
    return false;
}

static char *walk_files_(Arena *arena, t_args *args, DIR *dir, t_path *path) {
    ASSERT_(args, "args can not be NULL");
    ASSERT_(dir, "dir can not be NULL");
    ASSERT_(path, "path can not be NULL");

    errno = 0;
    const struct dirent *dirent = readdir(dir);

    while (dirent) {
        errno = 0;

        if (*dirent->d_name == '.' && !args->all) {
            dirent = readdir(dir);
            continue;
        }

        char fullpath[PATH_MAX] = {0};
        set_fullpath_(fullpath, path->name, dirent->d_name);

        struct stat sb;
        if (lstat(fullpath, &sb) == -1) {
            ft_fprintf(STDERR_FILENO, "ft_ls: cannot access: '%s': %s\n",
                       fullpath, strerror(errno));
            errno = 0;
        } else {
            if (S_ISDIR(sb.st_mode) && args->recursive) {
                if (!create_path_node_(args, path, dirent->d_name)) {
                    return strerror(errno);
                }
            }

            char *parse_error =
                parse_file_(arena, dirent, &sb, path, args->paths, fullpath);
            if (parse_error) {
                return parse_error;
            }
        }

        dirent = readdir(dir);
    }

    errno = 0;
    return NULL;
}

static char *parse_file_(Arena *arena, const struct dirent *dirent,
                         struct stat *sb, t_path *path, t_array *paths, const char *fullpath) {
    ASSERT_(dirent, "dirent can not be NULL");
    ASSERT_(sb, "sb can not be NULL");
    ASSERT_(path, "path can not be NULL");

    const size_t len = ft_strlen(dirent->d_name);
    if (len > path->max_len) {
        path->max_len = len;
    }

    t_file *file = ArenaPush(paths->arena, sizeof(*file));
    if (!file) {
        return strerror(errno);
    }

    if (!get_dt_(file->date_fmt, sb)) {
        return strerror(errno);
    }

    if (!get_symlink_(file->linkedname, fullpath, sb)) {
        return strerror(errno);
    }

    file->mtime = get_time_spec_(sb);
    get_permission_(file, sb);
    get_user_group_(arena, file, sb->st_gid, sb->st_uid);
    file->size = sb->st_size;
    file->blocks = (unsigned long)sb->st_blocks;
    file->hardlink = sb->st_nlink;
    file->filename_len = len;
    set_filename(file, dirent->d_name, path);


    if (!append_array(path->files, (void *)file)) {
        return strerror(errno);
    }

    return NULL;
}

static bool create_path_node_(t_args *args, const t_path *path,
                              const char *pathname) {
    ASSERT_(args, "args can not be NULL");
    ASSERT_(path, "path can not be NULL");
    ASSERT_(pathname, "pathname can not be NULL");
    ASSERT_(*pathname, "*pathname can not be '\\0'");

    if (*pathname == '.') {
        return true;
    }

    if (ft_strncmp(pathname, "..", ft_strlen(pathname)) == 0) {
        return true;
    }

    t_path *sub_path = ArenaPush(args->paths->arena, sizeof(*path));
    if (!sub_path) {
        return false;
    }

    sub_path->files = init_array(args->paths->arena, DEFAULT_SIZE, ARRAY_FILES);
    if (!sub_path->files) {
        goto failed;
    }

    set_fullpath_(sub_path->name, path->name, pathname);
    struct stat sb;
    if (lstat(sub_path->name, &sb) == -1) {
        ft_fprintf(STDERR_FILENO, "ft_ls: cannot access: '%s': %s\n",
                   sub_path->name, strerror(errno));
        errno = 0;
        return true;
    }

    sub_path->mtime = get_time_spec_(&sb);
    if (!append_array(args->paths, (void *)sub_path)) {
        goto failed;
    }

    return true;
failed:
    return false;
}

static void set_fullpath_(char *fullpath, const char *filename,
                          const char *dir_name) {
    ASSERT_(fullpath, "fullpath can not be NULL");
    ASSERT_(!*fullpath, "*fullpath must be '\\0'");
    ASSERT_(filename, "filename can not be NULL");
    ASSERT_(*filename, "*filename can not be '\\0'");
    ASSERT_(dir_name, "dir_name can not be NULL");
    ASSERT_(*dir_name, "*dir_name can not be '\\0'");

    const size_t len = ft_strlen(filename);

    size_t cpy_len = ft_strlcpy(fullpath, filename, PATH_MAX);
    if (filename[len - 1] != '/') {
        cpy_len += ft_strlcpy(fullpath + cpy_len, "/", PATH_MAX);
    }

    ft_strlcpy(fullpath + cpy_len, dir_name, PATH_MAX);
}

static void get_user_group_(Arena *arena, t_file *file, gid_t group_id,
                            uid_t user_id) {
    ASSERT_(file, "file can not be NULL");

    char *id = ArenaPush(arena, (U64)USER_SIZE * 2);
    if (!id) {
        // TODO: handle error
        return;
    }

    size_t len = 0;
    if (user_id != cached_uid) {
        const struct passwd *pwd = getpwuid(user_id);
        if (pwd) {
            ft_strlcpy(cached_user, pwd->pw_name, sizeof(cached_user));
        } else {
            len = uitoa(id, USER_SIZE, user_id);
            ft_strlcpy(cached_user, id, sizeof(cached_user));
        }
        cached_uid = user_id;
    }
    ft_strlcpy(file->user, cached_user, USER_SIZE);

    if (group_id != cached_gid) {
        const struct group *grp = getgrgid(group_id);
        if (grp) {
            ft_strlcpy(cached_group, grp->gr_name, sizeof(cached_group));
        } else {
            uitoa(id, USER_SIZE, group_id);
            ft_strlcpy(cached_group, id + len, sizeof(cached_group));
        }
        cached_gid = group_id;
    }
    ft_strlcpy(file->group, cached_group, USER_SIZE);
    ArenaClear(arena);
}

static void get_permission_(t_file *file, const struct stat *sb) {
    ASSERT_(file, "file can not be NULL");
    ASSERT_(sb, "sb can not be NULL");

    size_t len = 0;
    switch (sb->st_mode & S_IFMT) {
        case S_IFLNK:
            len += ft_strlcpy(file->permission + len, "l", PERMISSION_SIZE);
            break;
        case S_IFREG:
            len += ft_strlcpy(file->permission + len, "-", PERMISSION_SIZE);
            break;
        case S_IFDIR:
            len += ft_strlcpy(file->permission + len, "d", PERMISSION_SIZE);
            break;
        default:
            break;
    }

    len += ft_strlcpy(file->permission + len,
                      (sb->st_mode & S_IRUSR) ? "r" : "-", PERMISSION_SIZE);
    len += ft_strlcpy(file->permission + len,
                      (sb->st_mode & S_IWUSR) ? "w" : "-", PERMISSION_SIZE);
    len += ft_strlcpy(file->permission + len,
                      (sb->st_mode & S_IXUSR) ? "x" : "-", PERMISSION_SIZE);
    len += ft_strlcpy(file->permission + len,
                      (sb->st_mode & S_IRGRP) ? "r" : "-", PERMISSION_SIZE);
    len += ft_strlcpy(file->permission + len,
                      (sb->st_mode & S_IWGRP) ? "w" : "-", PERMISSION_SIZE);
    len += ft_strlcpy(file->permission + len,
                      (sb->st_mode & S_IXGRP) ? "x" : "-", PERMISSION_SIZE);
    len += ft_strlcpy(file->permission + len,
                      (sb->st_mode & S_IROTH) ? "r" : "-", PERMISSION_SIZE);
    len += ft_strlcpy(file->permission + len,
                      (sb->st_mode & S_IWOTH) ? "w" : "-", PERMISSION_SIZE);
    len += ft_strlcpy(file->permission + len,
                      (sb->st_mode & S_IXOTH) ? "x" : "-", PERMISSION_SIZE);
}

static bool get_dt_(char *buffer, const struct stat *sb) {
    ASSERT_(sb, "sb cannot be NULL");

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

    return true;
}

static struct timespec get_time_spec_(const struct stat *sb) {
    ASSERT_(sb, "sb cannot be NULL");

#if defined(__linux__)
    return sb->st_mtim;
#elif defined(__APPLE__)
    return sb->st_mtimespec;
#else
    ft_fprintf(STDERR_FILENO, "OS is not supported\n");
    return NULL;
#endif
}

static void set_filename(t_file *file, const char *filename, t_path *path) {
    ASSERT_(file, "file can not be NULL");
    ASSERT_(file->filename_len, "file->len must be more then 0");
    ASSERT_(filename, "filename can not be NULL");
    ASSERT_(*filename, "*filename can not be '\\0'");
    ASSERT_(path, "path can not be NULL");

    const char targets[4] = " '\"";
    const char *c = NULL;
    char quote[2] = "'";
    size_t index = 0;

    while (targets[index]) {
        c = ft_memchr(filename, targets[index], file->filename_len);
        if (c) {
            if (*c == '\'') {
                *quote = '"';
            }
            break;
        }

        ++index;
    }

#if defined(__linux__)
    if (c) {
        size_t len = 0;
        len += ft_strlcpy(file->filename, quote, NAME_MAX);
        len += ft_strlcpy(file->filename + len, filename, NAME_MAX);
        len += ft_strlcpy(file->filename + len, quote, NAME_MAX);

        ASSERT_(len == ft_strlen(file->filename), "len is not == to strlen()");
        file->filename_len = len;
        path->quoted = true;
        return;
    }
#endif
    (void)ft_strlcpy(file->filename, filename, NAME_MAX);
}

static bool get_symlink_(char *buffer, const char *filename, struct stat *sb) {
    if (S_ISLNK(sb->st_mode)) {
        ssize_t len = readlink(filename, buffer, NAME_MAX - 1);
        if (len < 0) {
            return false;
        }
    }

    return true;
}
