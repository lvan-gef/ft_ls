#include <errno.h>
#include <dirent.h>
#include <string.h>

#include "../include/ft_print.h"
#include "../include/ft_assert.h"
#include "../include/ft_fprintf.h"

static char *walk_files_(t_args *args, t_path *path, DIR *dir);

void printer(t_args *args) {
    ASSERT_(args, "args can not be NULL");

    char *err_msg = NULL;
    DIR *dir = NULL;

    size_t index = 0;
    while (index < args->paths->len) {
        errno = 0;
        t_path *path = args->paths->data[index];
        dir = opendir(path->name->str);
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

        err_msg = walk_files_(args, path, dir);
        if (err_msg || errno) {
            goto failed;
        }

        // print_();

        closedir(dir);
        ++index;
    }

    return;

failed:
    ft_fprintf(STDERR_FILENO, "errno: %d, %s\n", errno, err_msg);
    if (dir) {
        closedir(dir);
    }
    return;

}

static char *walk_files_(t_args *args, t_path *path, DIR *dir) {
    ASSERT_(args, "args can not be NULL");
    ASSERT_(path, "path can not be NULL");
    ASSERT_(dir, "dir can not be NULL");

    errno = 0;
    const struct dirent *dirent = readdir(dir);

    while (dirent) {
        errno = 0;

        if (*dirent->d_name == '.' && args->all) {
            // add hidden file
        } else {
            ft_fprintf(STDOUT_FILENO, "d_name: %s\n", dirent->d_name);
            ft_fprintf(STDOUT_FILENO, "d_name: %s\n", dirent->d_type);
            // add the rest
            // if recursive then add it to paths
        }

        dirent = readdir(dir);
    }

    errno = 0;
    return NULL;
}
