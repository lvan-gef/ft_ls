#ifndef FT_PRINTER_H
#define FT_PRINTER_H

#include <stdbool.h>
#include <stdint.h>

#include "./ft_array.h"
#include "./ft_str.h"

typedef struct {
    const t_array *entries;
    const t_str *dir_header;
    t_str *buffer;
    uint64_t min_len_links;
    uint64_t min_len_sizes;
    bool list_mode;
    bool print_total;
    bool quote_padding;
} t_print_request;

bool printer(const t_print_request *req);

#endif // !FT_PRINTER_H
