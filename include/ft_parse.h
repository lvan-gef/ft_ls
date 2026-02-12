#ifndef FT_PARSE_H
#define FT_PARSE_H

#include <stdbool.h>
#include <stdint.h>

#include "../include/ft_arena.h"
#include "../include/ft_array.h"

typedef struct {
    bool list;
    bool recursive;
    bool all;
    bool reverse;
    bool time;
} t_args;

t_array *parse_args(Arena *arena, uint64_t argc, char **argv, t_args *args);

#endif // !FT_PARSE_H
