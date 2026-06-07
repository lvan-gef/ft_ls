#ifndef FT_PRINTER_H
#define FT_PRINTER_H

#include <stdint.h>

#include "./ft_entry.h"
#include "./ft_str.h"
#include "./ft_array.h"

#ifndef TERM_SIZE
#define TERM_SIZE 80
#endif // !TERM_SIZE

#if TERM_SIZE < 1
#error "TERM_SIZE must be at least 1"
#endif

typedef struct {
    uint64_t total;
    uint64_t max_len_links;
    uint64_t max_len_sizes;
    uint64_t max_len_perm;
    bool have_quote;
} t_list_stats;

typedef struct {
    t_array *array;
    t_entry *dir_entry;
    t_str *buffer;
    t_list_stats stats;
    uint64_t min_len_links;
    uint64_t min_len_sizes;
    bool print_total;
    bool quote_padding;
} t_ps;

void printer(t_ps *ps, bool list_mode);
void print_list(t_ps *ps);
bool put_dir_header(t_str *out, const t_entry *dir_entry);
void update_list_stats(t_list_stats *stats, const t_entry *entry);

#endif // !FT_PRINTER_H
