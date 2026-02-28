#ifndef FT_PRINTER_H
#define FT_PRINTER_H

#include <stdint.h>

#include "./ft_parse.h"
#include "./ft_str.h"

void printer(const t_args *args, t_array *array, const t_str *dir_path,
             bool print_total, uint64_t min_len_links, uint64_t min_len_sizes);

#endif // !FT_PRINTER_H
