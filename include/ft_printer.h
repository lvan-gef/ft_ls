#ifndef FT_PRINTER_H
#define FT_PRINTER_H

#include <stdint.h>

#include "./ft_entry.h"
#include "./ft_parse.h"

#ifndef TERM_SIZE
#define TERM_SIZE 80
#endif // !TERM_SIZE

#if TERM_SIZE < 1
#error "TERM_SIZE must be at least 1"
#endif

typedef struct {
    t_args *args;
    t_array *array;
    t_entry *dir_entry;
    bool print_total;
    uint64_t min_len_links;
    uint64_t min_len_sizes;
    bool quote_padding;
} t_ps;

void printer(t_ps *ps);
void print_list(t_ps *ps);

#endif // !FT_PRINTER_H
