#ifndef FT_HELPERS_H
#define FT_HELPERS_H

#include <stddef.h>

#include "./ft_arena.h"
#include "./ft_ls.h"

size_t uitoa(char *buffer, size_t buffer_len, size_t n);
size_t get_len(size_t n);
t_str *join_paths(Arena *arena, t_str *path, t_str *filename);

#endif // !FT_HELPERS_H
