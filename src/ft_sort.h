#ifndef FT_SORT_H
#define FT_SORT_H

#include <stdbool.h>
#include <stdint.h>

typedef struct s_array t_array;

typedef struct {
    void **data;
    uint64_t cap;
} t_sort_scratch;

bool sort(t_sort_scratch *scratch, const t_array *array, bool reverse,
          bool sort_time);

#endif // !FT_SORT_H
