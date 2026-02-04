#ifndef FT_HELPERS_H
#define FT_HELPERS_H

#include <stddef.h>

#include "./ft_arena.h"
#include "./ft_ls.h"

size_t uitoa(char *buffer, size_t buffer_len, size_t n);
size_t get_len(size_t n);
char *left_pad(Arena *arena, size_t nbr, size_t nbr_len);
char *rigth_pad(Arena *arena, t_str *str, size_t len);
t_str *join_paths(Arena *arena, t_str *path, t_str *filename);

#endif // !FT_HELPERS_H
