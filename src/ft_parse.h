#ifndef FT_PARSE_H
#define FT_PARSE_H

#include <stdbool.h>
#include <stdint.h>

#include "./ft_array.h"
#include "./ft_str.h"

typedef struct s_args {
    bool list;
    bool recursive;
    bool all;
    bool reverse;
    bool time;
    bool no_owner;
    bool access_time;
    bool unsort;
    bool directory;
    bool no_group;
    bool color;
    bool sort_time;
} t_args;

bool parse_args(size_t argc, char **argv, t_args *args, t_array *inputs,
                t_str *views);

#endif /* ifndef FT_PARSE_H */
