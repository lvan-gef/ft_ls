#include "../include/ft_ls.h"
#include "../include/ft_assert.h"
#include "../include/ft_free.h"

void free_args(t_args *args) {
    CUSTOM_ASSERT_(args, "args can not be NULL");

    size_t index = 0;
    while (index < args->paths->len) {
        t_path *path = args->paths->data[index];
        free_array(path->files, free_file);

        ++index;
    }

    free_array(args->paths, free_path);
}

void free_file(void *content) {
    free(content);
}

void free_path(void *content) {
    free(content);
}
