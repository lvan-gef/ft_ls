#include "../include/ft_free.h"
#include "../include/ft_assert.h"
#include "../include/ft_ls.h"

void free_args(t_args *args) {
    ASSERT_(args, "args can not be NULL");

    size_t index = 0;
    while (index < args->paths->len) {
        t_path *path = args->paths->data[index];
        free_array(path->files);

        ++index;
    }

    if (args->paths) {
        free_array(args->paths);
    }
}

void free_file(void *content) {
    ASSERT_(content, "content can not be NULL");
    free(content);
}

void free_path(void *content) {
    ASSERT_(content, "content can not be NULL");
    free(content);
}
