#ifndef FT_PRINTER_LINUX_H
#define FT_PRINTER_LINUX_H

#if defined(__linux__)

#include <stdbool.h>
#include <stddef.h>

#include "./ft_ls.h"

bool print_linux(t_args *args, t_path *path, t_map *map, bool print_header);

#endif // __linux__

#endif // !FT_PRINTER_LINUX_H
