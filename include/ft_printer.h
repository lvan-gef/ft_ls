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
    Arena *arena;
    const t_array *quote_padding_context;
    const t_array *list_width_context;
    bool list_mode;
    bool print_total;
    bool quote_padding;
} t_print_request;

bool printer(const t_print_request *req);

#endif // !FT_PRINTER_H
