#ifndef FT_PARSE_H
#define FT_PARSE_H

#include <stdbool.h>
#include <stdint.h>

#include "./ft_array.h"

typedef struct {
    bool list;
    bool recursive;
    bool all;
    bool reverse;
    bool time;
} t_args;

bool parse_args(uint64_t argc, char **argv, t_args *args, t_array *inputs);

#endif // !FT_PARSE_H
