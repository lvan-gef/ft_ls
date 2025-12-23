#include "../include/ft_assert.h"
#include "../include/ft_ls.h"
#include "../libft/include/ft_fprintf.h"
#include "../libft/include/libft.h"

static void print_file_(void *content);
static void walk_paths_(void *content);

void print_ls(t_list *paths) {
    CUSTOM_ASSERT_(paths, "paths can not be NULL");

    ft_lstiter(paths, walk_paths_);
}

static void walk_paths_(void *content) {
    CUSTOM_ASSERT_(content, "content can not be NULL");

    t_path *path = content;
    ft_lstiter(path->files, print_file_);
}

static void print_file_(void *content) {
    CUSTOM_ASSERT_(content, "content can not be NULL");

    t_file *file = content;
    ft_fprintf(STDOUT_FILENO, "filename: %s\n", file->filename);
}
