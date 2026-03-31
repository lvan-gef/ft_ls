#ifndef FT_ARRAY_H
#define FT_ARRAY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "./ft_free_list.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE UINT64_C(10)
#endif // ifndef ARRAY_SIZE //

typedef struct s_array {
    uint64_t len;
    uint64_t cap;
    void **data;
    free_list *fl;
} t_array;

t_array *init_array(free_list *fl, uint64_t size);
bool append_array(t_array *array, void *content);
void *pop_array(t_array *array);
void reset_array(free_list *fl, t_array *array);

#endif // !FT_ARRAY_H
