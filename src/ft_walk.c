#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#include "../include/ft_walk.h"
#include "../include/ft_assert.h"
#include "../libft/include/ft_fprintf.h"
#include "../libft/include/libft.h"


static void clean_up_(DIR *dir);

bool walk(t_arguments *args) {
    CUSTOM_ASSERT_(args != NULL, "args can not be NULL");
    CUSTOM_ASSERT_(args->paths != NULL, "args->paths can not be NULL");
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
            clean_up_(dir);
            return false;
        }

        closedir(dir);
        ll = ll->next;
    }

    return true;
}

static void clean_up_(DIR *dir) {
    CUSTOM_ASSERT_(dir != NULL, "dir can not be NULL");

    closedir(dir);
}
