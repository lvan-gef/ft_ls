#ifndef FT_PARSE_H
#define FT_PARSE_H

#include <stdbool.h>
#include <stdint.h>

#include "../include/ft_array.h"
#include "../include/ft_free_list.h"

typedef struct {
    bool list;
    bool recursive;
    bool all;
    bool reverse;
    bool time;
} t_args;

t_array *parse_args(free_list *fl, uint64_t argc, char **argv, t_args *args);

#endif // !FT_PARSE_H
