#ifndef FT_SORT_H
#define FT_SORT_H

#include <stdbool.h>
#include <stddef.h>

#include "./ft_array.h"

void sort_alpha(t_array *files);
void sort_time(t_array *files, bool reverse);

#endif // !FT_SORT_H
