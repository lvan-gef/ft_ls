#ifndef FT_PRINTER_H
#define FT_PRINTER_H

#include <stdbool.h>

#include "./ft_array.h"
#include "./ft_str.h"

typedef struct s_print_request {
    const t_array *entries;
    const t_array *quote_padding_context;
    const t_array *list_width_context;
    const t_str *dir_header;
    t_str *buffer;
    bool list_mode;
    bool print_total;
} t_print_request;

bool printer(const t_print_request *req);

#endif // !FT_PRINTER_H
