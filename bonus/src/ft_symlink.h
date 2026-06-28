#ifndef FT_SYMLINK_H
#define FT_SYMLINK_H

#include <stdint.h>

#include "../include/ft_str.h"

#include "./ft_arena.h"

typedef struct s_arena Arena;
typedef struct s_str t_str;

t_str *read_symlink(Arena *scratch, const t_str *path, uint64_t target_size,
                    int *read_err);

#endif // !FT_SYMLINK_H
