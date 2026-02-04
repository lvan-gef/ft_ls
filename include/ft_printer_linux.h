#ifndef FT_PRINTER_LINUX_H
#define FT_PRINTER_LINUX_H

#include "ft_arena.h"
#include "ft_print_list.h"
#if defined(__linux__)

#include <stdbool.h>
#include <stddef.h>

#include "./ft_ls.h"

bool print_linux(t_args *args, t_path *path, t_map *map, bool print_header);
bool linux_list_format(Arena *arena, t_file_list *fl, t_file *file, char **output_str);

#endif // __linux__

#endif // !FT_PRINTER_LINUX_H
