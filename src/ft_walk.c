#include <dirent.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_free.h"
#include "../include/ft_ls.h"
#include "../include/ft_walk.h"
#include "../libft/include/ft_fprintf.h"
#include "../libft/include/libft.h"

static char *walk_files(t_args *args, DIR *dir, t_path *path);
static char *parse_file(struct dirent *dirent, struct stat *sb, t_path *path);
static bool create_path_node_(t_args *args, const t_path *path,
                              const char *pathname);
static void set_fullpath_(char *fullpath, const char *filename,
                          const char *dir_name);
static void get_user_group_(t_file *file, unsigned int group_id,
                            unsigned int user_id);
static void get_permission_(t_file *file, struct stat *sb);

static uid_t cached_uid = (uid_t)-1;
static gid_t cached_gid = (gid_t)-1;
static char cached_user[32];
static char cached_group[32];

bool walk(t_args *args) {
    CUSTOM_ASSERT_(args, "args can not be NULL");
    CUSTOM_ASSERT_(args->paths, "args->paths can not be NULL");

    DIR *dir = NULL;
    char *err_msg = NULL;
    size_t index = 0;

    while (index < args->paths->len) {
        errno = 0;
        t_path *path = args->paths->data[index];
        dir = opendir(path->path);
        if (!dir) {
            if (errno == EACCES) {
                ft_fprintf(STDERR_FILENO, "ft_ls: cannot access: '%s': %s\n",
                           path->path, strerror(errno));
                errno = 0;
                ++index;
                continue;
            }

            if (errno == ENOENT) {
                errno = 0;
                ++index;
                continue;
            }

            ft_fprintf(STDOUT_FILENO, "%s\n", path->path);
            err_msg = strerror(errno);
            goto failed;
        }

        err_msg = walk_files(args, dir, path);
        if (err_msg || errno) {
            goto failed;
        }

        closedir(dir);
        ++index;
    }

    return true;
failed:
    ft_fprintf(STDERR_FILENO, "errno: %d, %s\n", errno, err_msg);
    if (dir) {
        closedir(dir);
    }
    return false;
}

// TODO: . and .. we need that for ls even if not in recursive mode
static char *walk_files(t_args *args, DIR *dir, t_path *path) {
    CUSTOM_ASSERT_(args, "args can not be NULL");
    CUSTOM_ASSERT_(dir, "dir can not be NULL");
    CUSTOM_ASSERT_(path, "path can not be NULL");

    errno = 0;
    struct dirent *dirent = readdir(dir);

    while (dirent) {
        errno = 0;
        struct stat sb;

        if (*dirent->d_name == '.' && !args->all) {
            dirent = readdir(dir);
            continue;
        }

        char fullpath[PATH_MAX] = {0};
        set_fullpath_(fullpath, path->path, dirent->d_name);

        if (lstat(fullpath, &sb) == -1) {
            ft_fprintf(STDERR_FILENO, "ft_ls: cannot access: '%s': %s\n",
                       fullpath, strerror(errno));
            errno = 0;
            dirent = readdir(dir);
            continue;
        }

        if (S_ISDIR(sb.st_mode) && args->recursive) {
            if (!create_path_node_(args, path, dirent->d_name)) {
                return strerror(errno);
            }

            dirent = readdir(dir);
            continue;
        }

        char *parse_error = parse_file(dirent, &sb, path);
        if (parse_error) {
            return parse_error;
        }

        dirent = readdir(dir);
    }

    errno = 0;
    return NULL;
}

static char *parse_file(struct dirent *dirent, struct stat *sb, t_path *path) {
    CUSTOM_ASSERT_(dirent, "dirent can not be NULL");
    CUSTOM_ASSERT_(sb, "sb can not be NULL");
    CUSTOM_ASSERT_(path, "path can not be NULL");

    const size_t len = ft_strlen(dirent->d_name);
    if (len > path->max_len) {
        path->max_len = len;
    }

    const char *dt = ctime(&sb->st_atim.tv_sec);
    if (!dt) {
        return strerror(errno);
    }

    t_file *file = ft_calloc(1, sizeof(*file));
    if (!file) {
        return strerror(errno);
    }

    ft_strlcpy(file->date_fmt, dt + 4, DT_LEN);
    ft_strlcpy(file->filename, dirent->d_name, NAME_MAX);
    get_permission_(file, sb);
    get_user_group_(file, sb->st_gid, sb->st_uid);
    file->size = sb->st_size;
    file->hardlink = sb->st_nlink;
    file->len = len;

    if (!append_array(path->files, (void *)file)) {
        free(file);
        return strerror(errno);
    }

    return NULL;
}

static bool create_path_node_(t_args *args, const t_path *path,
                              const char *pathname) {
    CUSTOM_ASSERT_(args, "args can not be NULL");
    CUSTOM_ASSERT_(path, "path can not be NULL");
    CUSTOM_ASSERT_(pathname, "pathname can not be NULL");
    CUSTOM_ASSERT_(*pathname != '\0', "*pathname can not be '\\0'");

    if (*pathname == '.') {
        return true;
    }

    if (ft_strncmp(pathname, "..", ft_strlen(pathname)) == 0) {
        return true;
    }

    t_path *sub_path = ft_calloc(1, sizeof(*path));
    if (!sub_path) {
        return false;
    }

    sub_path->files = init_array(10);
    if (!sub_path->files) {
        free(sub_path);
        return false;
    }

    set_fullpath_(sub_path->path, path->path, pathname);
    if (!append_array(args->paths, (void *)sub_path)) {
        free_array(sub_path->files, free_file);
        free(sub_path);
        return false;
    }

    return true;
}

static void set_fullpath_(char *fullpath, const char *filename,
                          const char *dir_name) {
    CUSTOM_ASSERT_(fullpath, "fullpath can not be NULL");
    CUSTOM_ASSERT_(*fullpath == '\0', "*fullpath must be '\\0'");
    CUSTOM_ASSERT_(filename, "filename can not be NULL");
    CUSTOM_ASSERT_(*filename != '\0', "*filename can not be '\\0'");
    CUSTOM_ASSERT_(dir_name, "dir_name can not be NULL");
    CUSTOM_ASSERT_(*dir_name != '\0', "*dir_name can not be '\\0'");

    const size_t len = ft_strlen(filename);

    ft_strlcpy(fullpath, filename, PATH_MAX);
    if (filename[len - 1] != '/') {
        ft_strlcat(fullpath, "/", PATH_MAX);
    }
    ft_strlcat(fullpath, dir_name, PATH_MAX);
}

static void get_user_group_(t_file *file, gid_t group_id, uid_t user_id) {
    if (user_id != cached_uid) {
        struct passwd *pwd = getpwuid(user_id);
        if (pwd) {
            ft_strlcpy(cached_user, pwd->pw_name, sizeof(cached_user));
            cached_uid = user_id;
        }
    }
    ft_strlcpy(file->user, cached_user, USER_SIZE);

    if (group_id != cached_gid) {
        struct group *grp = getgrgid(group_id);
        if (grp) {
            ft_strlcpy(cached_group, grp->gr_name, sizeof(cached_group));
            cached_gid = group_id;
        }
    }
    ft_strlcpy(file->group, cached_group, USER_SIZE);
}

static void get_permission_(t_file *file, struct stat *sb) {
    CUSTOM_ASSERT_(file, "file can not be NULL");
    CUSTOM_ASSERT_(sb, "sb can not be NULL");

    switch (sb->st_mode & S_IFMT) {
        case S_IFLNK:
            // ft_fprintf(STDOUT_FILENO, "symbolic link\n");
            ft_strlcat(file->permission, "l", PERMISSION_SIZE);
            break;
        case S_IFREG:
            ft_strlcat(file->permission, "-", PERMISSION_SIZE);
            break;
        case S_IFDIR:
            ft_strlcat(file->permission, "d", PERMISSION_SIZE);
            break;
        default:
            // ft_fprintf(STDOUT_FILENO, "others have to check it\n");
            break;
    }

    ft_strlcat(file->permission, (sb->st_mode & S_IRUSR) ? "r" : "-",
               PERMISSION_SIZE);
    ft_strlcat(file->permission, (sb->st_mode & S_IWUSR) ? "w" : "-",
               PERMISSION_SIZE);
    ft_strlcat(file->permission, (sb->st_mode & S_IXUSR) ? "x" : "-",
               PERMISSION_SIZE);
    ft_strlcat(file->permission, (sb->st_mode & S_IRGRP) ? "r" : "-",
               PERMISSION_SIZE);
    ft_strlcat(file->permission, (sb->st_mode & S_IWGRP) ? "w" : "-",
               PERMISSION_SIZE);
    ft_strlcat(file->permission, (sb->st_mode & S_IXGRP) ? "x" : "-",
               PERMISSION_SIZE);
    ft_strlcat(file->permission, (sb->st_mode & S_IROTH) ? "r" : "-",
               PERMISSION_SIZE);
    ft_strlcat(file->permission, (sb->st_mode & S_IWOTH) ? "w" : "-",
               PERMISSION_SIZE);
    ft_strlcat(file->permission, (sb->st_mode & S_IXOTH) ? "x" : "-",
               PERMISSION_SIZE);
}
