#ifndef FT_PRINTER_H
#define FT_PRINTER_H

#include <stdint.h>

#include "./ft_parse.h"
#include "./ft_str.h"

void printer(t_args *args, t_array *array, t_str *dir_path, bool print_total);

#endif // !FT_PRINTER_H
