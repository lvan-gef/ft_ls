#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#include "../include/ft_walk.h"
#include "../libft/include/ft_fprintf.h"
#include "../libft/include/libft.h"

bool walk(t_arguments *args) {
    t_list *ll = args->paths;

    while (ll) {
        t_node *node = ll->content;
        DIR *dir = opendir((char *)node->path);
        if (!dir) {
            ft_fprintf(STDERR_FILENO, "Failed to open the dir\n");
            return false;
        }

        errno = 0;
        struct dirent *dirent = readdir(dir);
        while (dirent && !errno) {
            ft_fprintf(STDOUT_FILENO, "%s\n", dirent->d_name);
            dirent = readdir(dir);
        }

        if (errno) {
            ft_fprintf(STDERR_FILENO, "Failed to read the dir\n");
            return false;
        }

        closedir(dir);
        ll = ll->next;
    }

    return true;
}
