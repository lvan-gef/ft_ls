#ifndef FT_PRINTER_LIST_H
#define FT_PRINTER_LIST_H

#include <stdbool.h>
#include <stdint.h>


typedef struct s_print_request t_print_request;

bool printer_list(const t_print_request *req);

#endif // !FT_PRINTER_LIST_H
