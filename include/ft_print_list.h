#ifndef FT_PRINT_LIST_H
#define FT_PRINT_LIST_H

#include <stdbool.h>
#include <stdint.h>

#include "./ft_array.h"
#include "./ft_str.h"

void print_list(t_array *array, const t_str *path, bool print_total,
                uint64_t min_len_links, uint64_t min_len_sizes);

#endif // !FT_PRINT_LIST_H
