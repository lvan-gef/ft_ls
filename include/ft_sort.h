#ifndef FT_SORT_H
#define FT_SORT_H

#include <stdbool.h>
#include <stdint.h>

#include "./ft_array.h"

typedef struct {
    void **data;
    uint64_t cap;
} t_sort_scratch;

void sort(t_sort_scratch *scratch, t_array *array, bool reverse,
          bool sort_time);

#endif // !FT_SORT_H
