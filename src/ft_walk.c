#include <dirent.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "../include/ft_assert.h"
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
static void free_file_(void *content);

// TODO: paths get messed up with / when it starts or ands with it as argument

bool walk(t_args *args, t_list **paths) {
    CUSTOM_ASSERT_(args, "args can not be NULL");
    CUSTOM_ASSERT_(args->paths, "args->paths can not be NULL");
    CUSTOM_ASSERT_(paths, "paths can not be NULL");
    CUSTOM_ASSERT_(!*paths, "*paths must be NULL");

    DIR *dir = NULL;
    char *err_msg = {0};
    t_list *ll = args->paths;

    while (ll) {
        t_node *node = ll->content;
        dir = opendir(node->path);
        if (!dir) {
            if (errno == 13) {
                // ls: kan map '/var/lib/AccountsService/users' niet openen: Toegang geweigerd
                ft_fprintf(STDERR_FILENO,
                           "ls: can map: '%s' not open: Permission denied",
                           node->path);
                errno = 0;
                ll = ll->next;
                continue;
            }

            ft_fprintf(STDOUT_FILENO, "%s ,", node->path);
            err_msg = strerror(errno);
            goto failed;
        }

        t_path *path = ft_calloc(1, sizeof(*path));
        if (!path) {
            err_msg = "Failed to calloc path struct";
            goto failed;
        }
        ft_strlcpy(path->path, node->path, MAX_PATH);

        err_msg = walk_files(args, dir, path);
        if (*err_msg) {
            goto failed;
        }

        t_list *p_node = ft_lstnew((void *)path);
        if (!p_node) {
            err_msg = "Failed to create a node for t_path";
            goto failed;
        }
        ft_lstadd_back(paths, p_node);

        closedir(dir);
        ll = ll->next;
    }

    return true;
failed:
    ft_fprintf(STDERR_FILENO, "errno: %d, %s\n", errno, err_msg);
    if (dir) {
        closedir(dir);
    }
    return false;
}

void free_walk(void *content) {
    CUSTOM_ASSERT_(content, "content can not be NULL");

    t_path *path = (t_path *)content;
    ft_lstclear(&path->files, free_file_);
    free(path);
}

// TODO: . and .. we need that for ls even if not in recursive mode
static char *walk_files(t_args *args, DIR *dir, t_path *path) {
    CUSTOM_ASSERT_(args, "args can not be NULL");
    CUSTOM_ASSERT_(dir, "dir can not be NULL");
    CUSTOM_ASSERT_(path, "path can not be NULL");

    errno = 0;
    struct dirent *dirent = readdir(dir);

    while (dirent && !errno) {
        struct stat sb;

        if (*dirent->d_name == '.' && !args->all) {
            dirent = readdir(dir);
            continue;
        }

        char fullpath[MAX_PATH] = {0};
        set_fullpath_(fullpath, path->path, dirent->d_name);

        if (lstat(fullpath, &sb) == -1) {
            return strerror(errno);
        }

        if (S_ISDIR(sb.st_mode) && args->recursive) {
            if (!create_path_node_(args, path, dirent->d_name)) {
                return strerror(errno);
            }

            dirent = readdir(dir);
            continue;
        }

        char *parse_error = parse_file(dirent, &sb, path);
        if (*parse_error) {
            return parse_error;
        }
        ++path->file_count;

        dirent = readdir(dir);
    }

    if (errno) {
        return strerror(errno);
    }

    return "\0";
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

    ft_strlcpy(file->date, dt + 4, DT_LEN);
    ft_strlcpy(file->filename, dirent->d_name, MAX_PATH);
    get_permission_(file, sb);
    get_user_group_(file, sb->st_gid, sb->st_uid);
    file->size = sb->st_size;
    file->hardlink = sb->st_nlink;
    file->len = len;

    t_list *node = ft_lstnew((void *)file);
    if (!node) {
        free(file);
        return strerror(errno);
    }
    ft_lstadd_back(&path->files, node);

    return "\0";
}

static bool create_path_node_(t_args *args, const t_path *path,
                              const char *pathname) {
    CUSTOM_ASSERT_(args, "args can not be NULL");
    CUSTOM_ASSERT_(path, "path can not be NULL");
    CUSTOM_ASSERT_(pathname, "pathname can not be NULL");
    CUSTOM_ASSERT_(*pathname != '\0', "*pathname can not be '\\0'");

    if (*pathname == '.' && !args->all) {
        return true;
    }

    if (*pathname == '.' ||
        ft_strncmp(pathname, "..", ft_strlen(pathname)) == 0) {
        return true;
    }

    t_path *sub_path = ft_calloc(1, sizeof(*path));
    if (!path) {
        return false;
    }

    ft_strlcpy(sub_path->path, path->path, MAX_PATH);
    ft_strlcat(sub_path->path, "/", MAX_PATH);
    ft_strlcat(sub_path->path, pathname, MAX_PATH);
    t_list *node = ft_lstnew((void *)sub_path);
    if (!node) {
        free(sub_path);
        return false;
    }
    ft_lstadd_back(&args->paths, node);

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

    ft_strlcpy(fullpath, filename, MAX_PATH);
    ft_strlcat(fullpath, "/", MAX_PATH);
    ft_strlcat(fullpath, dir_name, MAX_PATH);
}

static void get_user_group_(t_file *file, unsigned int group_id,
                            unsigned int user_id) {
    CUSTOM_ASSERT_(file, "file can no be NULL");

    struct group *grp = getgrgid(group_id);
    if (grp) {
        ft_strlcpy(file->group, grp->gr_name, MAX_PATH);
    } else {
        // TODO: handle error like ls
    }

    struct passwd *pwd = getpwuid(user_id);
    if (pwd) {
        ft_strlcpy(file->user, pwd->pw_name, MAX_PATH);
    } else {
        // TODO: handle error like ls
    }
}

static void get_permission_(t_file *file, struct stat *sb) {
    CUSTOM_ASSERT_(file, "file can not be NULL");
    CUSTOM_ASSERT_(sb, "sb can not be NULL");

    switch (sb->st_mode & S_IFMT) {
        case S_IFLNK:
            ft_fprintf(STDOUT_FILENO, "symbolic link\n");
            ft_strlcat(file->permission, "l", PERMISSION_SIZE);
            break;
        case S_IFREG:
            ft_strlcat(file->permission, "-", PERMISSION_SIZE);
            break;
        case S_IFDIR:
            ft_strlcat(file->permission, (sb->st_mode & S_IRUSR) ? "d" : "-",
                       PERMISSION_SIZE);
            break;
        default:
            ft_fprintf(STDOUT_FILENO, "others have to check it\n");
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

static void free_file_(void *content) {
    CUSTOM_ASSERT_(content, "content can not be NULL");

    t_file *file = content;
    free(file);
}
