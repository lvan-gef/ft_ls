#ifndef FT_PRINTER_MAC_H
#define FT_PRINTER_MAC_H

#if defined(__APPLE__)

#include <stdbool.h>
#include <stddef.h>

#include "./ft_ls.h"

bool print_mac(t_args *args, t_path *path, t_map *map, bool print_header,
               size_t queue_index);

#endif // __APPLE__

#endif // !FT_PRINTER_MAC_H
