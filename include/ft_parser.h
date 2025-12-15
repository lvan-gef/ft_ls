#ifndef FT_LS_PARSER_H
#define FT_LS_PARSER_H

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

bool parse_args(int argc, char **argv, t_arguments *args);
void free_args(t_arguments *args);

#endif // FT_LS_PARSER_H
