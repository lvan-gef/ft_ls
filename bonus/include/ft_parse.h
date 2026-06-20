#ifndef FT_PARSE_H
#define FT_PARSE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct s_array t_array;

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
} t_args;

bool parse_args(uint64_t argc, char **argv, t_args *args, t_array *inputs);

#endif // !FT_PARSE_H
