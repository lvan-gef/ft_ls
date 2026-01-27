#ifndef FT_PARSER_H
#define FT_PARSER_H

#include <stdbool.h>

#include "./ft_arena.h"

#include "../include/ft_ls.h"

bool parse_args(Arena *arena, int argc, char **argv, t_args *args);
bool default_arg(t_args *args);

#endif // !FT_PARSER_H
