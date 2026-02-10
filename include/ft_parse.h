#ifndef FT_PARSE_H
#define FT_PARSE_H

#include "../include/ft_arena.h"
#include "../include/ft_array.h"
#include <stdbool.h>

typedef struct {
    bool list;
    bool recursive;
    bool all;
    bool reverse;
    bool time;
} t_args;

t_array *parse_args(Arena *arena, int argc, char **argv, t_args *args);

#endif // !FT_PARSE_H
