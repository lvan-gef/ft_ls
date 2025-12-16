#ifndef FT_LS_H
#define FT_LS_H

#include <stdbool.h>

#include "../libft/include/libft.h"

typedef struct s_arguments {
    bool list;
    bool recursive;
    bool all;
    bool reverse;
    bool time;
    t_list *paths;
} t_arguments;

typedef struct s_node {
    char *path;
} t_node;


#endif // FT_LS_H
