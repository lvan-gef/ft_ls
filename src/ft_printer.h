#ifndef FT_PRINTER_H
#define FT_PRINTER_H

#include <stdbool.h>

typedef struct s_array t_array;
typedef struct s_str t_str;

typedef struct s_print_request {
    const t_array *entries;
    const t_array *list_width_context;
    const t_str *dir_header;
    t_str *buffer;
    bool quote_padding;
    bool list_mode;
    bool print_total;
} t_print_request;

bool printer(const t_print_request *req);
bool printer_list(const t_print_request *req);

#endif // !FT_PRINTER_H
