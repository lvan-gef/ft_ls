#ifndef FT_LS_H
#define FT_LS_H

#include <stdbool.h>

#include "../libft/include/libft.h"

#ifndef MAX_PATH
#define MAX_PATH 256
#endif // !MAX_PATH

typedef struct s_args {
    bool list;
    bool recursive;
    bool all;
    bool reverse;
    bool time;
    t_list *paths;
} t_args;

typedef struct s_path {
    char path[MAX_PATH];
    size_t max_len;
    t_list *files;
} t_path;

typedef struct s_node {
    char path[MAX_PATH];
} t_node;

#endif // FT_LS_H
