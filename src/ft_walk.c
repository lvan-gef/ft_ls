#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "../include/ft_assert.h"
#include "../include/ft_ls.h"
#include "../include/ft_walk.h"
#include "../libft/include/ft_fprintf.h"
#include "../libft/include/libft.h"

static char *parse_files(t_args *args, DIR *dir, t_path *path);
static void clean_up_(DIR *dir, t_list *paths);
static void free_file_(void *content);

bool walk(t_args *args, t_list *paths) {
    CUSTOM_ASSERT_(args, "args can not be NULL");
    CUSTOM_ASSERT_(args->paths, "args->paths can not be NULL");

    DIR *dir = NULL;
    char *err_msg = {0};
    t_list *ll = args->paths;

    while (ll) {
        t_node *node = ll->content;
        dir = opendir(node->path);
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

        closedir(dir);
        ll = ll->next;
    }

    return true;
failed:
    ft_fprintf(STDERR_FILENO, "errno: %d, %s\n", errno, err_msg);
    clean_up_(dir, paths);
    return false;
}

void free_path(void *content) {
    CUSTOM_ASSERT_(content, "content can not be NULL");

    t_path *path = (t_path *)content;
    ft_lstclear(&path->files, free_file_);
}

static char *parse_files(t_args *args, DIR *dir, t_path *path) {
    CUSTOM_ASSERT_(args, "args can not be NULL");
    CUSTOM_ASSERT_(dir, "dir can not be NULL");
    CUSTOM_ASSERT_(path, "path can not be NULL");

    errno = 0;
    struct dirent *dirent = readdir(dir);

    while (dirent && !errno) {
        // if -R create a node and add it to ll
        // if file create a node add stuff and apppend
        ft_fprintf(STDOUT_FILENO, "%s\n", dirent->d_name);

        const size_t len = ft_strlen(dirent->d_name);
        if (*dirent->d_name == '.' && !args->all) {
            dirent = readdir(dir);
            continue;
        }

        if (len > path->max_len) {
            path->max_len = len;
        }

        // get file info
        // add to struct
        // create node
        //

        dirent = readdir(dir);
    }

    if (errno) {
        return strerror(errno);
    }

    // append it to paths

    return "\0";
}

static void free_file_(void *content) {
    (void)content;
}

static void clean_up_(DIR *dir, t_list *paths) {
    CUSTOM_ASSERT_(paths, "paths can not be NULL");

    if (dir) {
        closedir(dir);
    }

    if (paths) {
        ft_lstclear(&paths, free_path);
    }
}
