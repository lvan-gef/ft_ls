#ifndef FT_HELPER_H
#define FT_HELPER_H

#include <stdint.h>

#include "../include/ft_arena.h"
#include "../include/ft_entry.h"

uint64_t len_of_nbr(uint64_t nbr);
t_entry *escape_entry(Arena *arena, t_entry *entry);
t_str *escape_str(Arena *arena, t_str *str);

#endif // !FT_HELPER_H
