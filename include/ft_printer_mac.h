#ifndef FT_PRINTER_MAC_H
#define FT_PRINTER_MAC_H

#if defined(__APPLE__)

#include <stdbool.h>
#include <stddef.h>

#include "./ft_ls.h"

bool print_cols_mac(t_path *path, size_t num_cols, size_t num_rows);
void calc_cols_mac(t_path *path, size_t *num_cols, size_t *num_rows);

#endif // __APPLE__

#endif // !FT_PRINTER_MAC_H
