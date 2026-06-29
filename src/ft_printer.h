#ifndef FT_PRINTER_H
#define FT_PRINTER_H

#include <stdbool.h>

#include "../include/ft_array.h"
#include "../include/ft_str.h"

#include "./ft_arena.h"

typedef struct s_array t_array;
typedef struct s_str t_str;

typedef struct s_print_request {
    const t_array *entries;
    const t_array *list_width_context;
    const t_str *dir_header;
    t_str *buffer;
    Arena *file_info_arena;
    bool quote_padding;
    bool list_mode;
    bool print_total;
} t_print_request;

bool printer(const t_print_request *req);
bool printer_list(const t_print_request *req);

#endif /* ifndef FT_PRINTER_H */
