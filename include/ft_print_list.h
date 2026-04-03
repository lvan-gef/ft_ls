#ifndef FT_PRINT_LIST_H
#define FT_PRINT_LIST_H

#include <stdbool.h>
#include <stdint.h>

#include "./ft_array.h"
#include "./ft_entry.h"

void print_list(t_array *array, const t_entry *dir_entry, bool print_total,
                uint64_t min_len_links, uint64_t min_len_sizes,
                bool force_quote_padding);

#endif // !FT_PRINT_LIST_H
