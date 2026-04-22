#ifndef FT_PRINTER_H
#define FT_PRINTER_H

#include <stdint.h>

#include "./ft_entry.h"
#include "./ft_parse.h"

#ifndef TERM_SIZE
#define TERM_SIZE 80
#endif // !TERM_SIZE

void printer(const t_args *args, t_array *array, const t_entry *dir_entry,
             bool print_total, uint64_t min_len_links, uint64_t min_len_sizes,
             bool force_quote_padding);

#endif // !FT_PRINTER_H
