#ifndef FT_PATH_SCRATCH_H
#define FT_PATH_SCRATCH_H

#include <stdint.h>

#include "../include/ft_str.h"

#include "ft_arena.h"

t_str *path_read_symlink_scratch(Arena *scratch, const t_str *path,
                                 uint64_t target_size, int *read_err);

#endif // !FT_PATH_SCRATCH_H
