#include <dirent.h>
#include <errno.h>
#include <grp.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <pwd.h>

#include "../include/ft_assert.h"
#include "../include/ft_ls.h"
#include "../include/ft_walk.h"
#include "../libft/include/ft_fprintf.h"
#include "../libft/include/libft.h"

static char *parse_files(t_args *args, DIR *dir, t_path *path);
static bool create_path_node_(t_args *args, const t_path *path, const char *pathname);
static void get_permission_(t_file *file, struct stat *sb);
static void clean_up_(DIR *dir, t_list **paths);
static void free_file_(void *content);

bool walk(t_args *args, t_list **paths) {
    CUSTOM_ASSERT_(args, "args can not be NULL");
    CUSTOM_ASSERT_(args->paths, "args->paths can not be NULL");
    CUSTOM_ASSERT_(paths, "paths can not be NULL");
    // CUSTOM_ASSERT_(*paths, "*paths can not be NULL");

    DIR *dir = NULL;
    char *err_msg = {0};
    t_list *ll = args->paths;

    while (ll) {
        t_node *node = ll->content;
        dir = opendir(node->path);
        ft_fprintf(STDOUT_FILENO, "--> %s\n", node->path);
        if (!dir) {
            err_msg = "Failed to open the dir";
            goto failed;
        }

        t_path *path = ft_calloc(1, sizeof(*path));
        if (!path) {
            err_msg = "Failed to calloc path struct";
            goto failed;
        }
        ft_strlcpy(path->path, node->path, MAX_PATH);

        err_msg = parse_files(args, dir, path);
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
    clean_up_(dir, paths);
    return false;
}

void free_walk(void *content) {
    CUSTOM_ASSERT_(content, "content can not be NULL");

    t_path *path = (t_path *)content;
    ft_lstclear(&path->files, free_file_);
    free(path);
}

static char *parse_files(t_args *args, DIR *dir, t_path *path) {
    CUSTOM_ASSERT_(args, "args can not be NULL");
    CUSTOM_ASSERT_(dir, "dir can not be NULL");
    CUSTOM_ASSERT_(path, "path can not be NULL");

    errno = 0;
    struct dirent *dirent = readdir(dir);
    t_list *node = NULL;

    while (dirent && !errno) {
        struct stat sb;

        char fullpath[MAX_PATH] = {0};

        ft_strlcpy(fullpath, path->path, MAX_PATH);
        ft_strlcat(fullpath, "/", MAX_PATH);
        ft_strlcat(fullpath, dirent->d_name, MAX_PATH);
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

        const size_t len = ft_strlen(dirent->d_name);
        if (*dirent->d_name == '.' && !args->all) {
            dirent = readdir(dir);
            continue;
        }

        if (len > path->max_len) {
            path->max_len = len;
        }

        t_file *file = ft_calloc(1, sizeof(*file));
        if (!file) {
            return strerror(errno);
        }

        struct group *grp = getgrgid(sb.st_gid);
        if (grp) {
            ft_strlcpy(file->group, grp->gr_name, MAX_PATH);
        } else {
            // TODO: handle error like ls
        }

        struct passwd *pwd = getpwuid(sb.st_uid);
        if (pwd) {
            ft_strlcpy(file->user, pwd->pw_name, MAX_PATH);
        } else {
            // TODO: handle error like ls
        }

        file->size = sb.st_size;
        ft_strlcpy(file->filename, dirent->d_name, MAX_PATH);
        get_permission_(file, &sb);
        file->hardlink = sb.st_nlink;
        ft_fprintf(STDOUT_FILENO, "%s %d %s %s %d %s\n", file->permission,
                   file->hardlink, file->user, file->group, file->size, file->filename);

        node = ft_lstnew((void *)file);
        if (!node) {
            free(file);
            return strerror(errno);
        }
        ft_lstadd_back(&path->files, node);

        dirent = readdir(dir);
    }

    if (errno) {
        return strerror(errno);
    }

    return "\0";
}

static bool create_path_node_(t_args *args, const t_path *path, const char *pathname) {
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

static void get_permission_(t_file *file, struct stat *sb) {
    switch (sb->st_mode & S_IFMT) {
        case S_IFLNK:
            ft_fprintf(STDOUT_FILENO, "symbolic link\n");
            break;
        case S_IFREG:
            ft_strlcat(file->permission, "-", PERMISSION_SIZE);
            break;
        case S_IFDIR:
            ft_strlcat(file->permission, (sb->st_mode & S_IRUSR) ? "d" : "-",
                       PERMISSION_SIZE);
            break;
        default:
            ft_fprintf(STDOUT_FILENO, "others have to check it");
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

static void clean_up_(DIR *dir, t_list **paths) {
    CUSTOM_ASSERT_(paths, "paths can not be NULL");

    if (dir) {
        closedir(dir);
    }

    ft_lstclear(paths, free_walk);
}
