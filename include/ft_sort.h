#ifndef FT_SORT_H
#define FT_SORT_H

#include <stdbool.h>

#include "./ft_arena.h"
#include "./ft_array.h"

void sort(Arena *arena, t_array *array, bool reverse, bool sort_time);

#endif // !FT_SORT_H
