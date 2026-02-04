#ifndef FT_PRINT_LIST_H
#define FT_PRINT_LIST_H

#include <stdbool.h>

#include "./ft_ls.h"

typedef struct {
    size_t *lens;
    size_t wb_len;
    size_t list_index;
    size_t buffer_len;
} t_file_list;

bool print_list(t_path *path, t_array *files);

#endif // !FT_PRINT_LIST_H
