#ifndef FT_PRINTER_LIST_SCRATCH_H
#define FT_PRINTER_LIST_SCRATCH_H

#include <stdint.h>

#include "../include/ft_str.h"

#include "ft_arena.h"

t_str *printer_scratch_str_new(Arena *scratch, uint64_t cap);
t_str *printer_scratch_str_from_cstr(Arena *scratch, const char *src);
t_str *printer_scratch_str_from_uint(Arena *scratch, uint64_t value);

#endif // !FT_PRINTER_LIST_SCRATCH_H
