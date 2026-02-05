#ifndef FT_GET_STATS_H
#define FT_GET_STATS_H

#include <stdbool.h>
#include <sys/stat.h>

#include "ft_arena.h"
#include "./ft_ls.h"

bool get_file_info(Arena *arena, struct stat *sb, t_file *file,const char *fullname);

#endif // !FT_GET_STATS_H
