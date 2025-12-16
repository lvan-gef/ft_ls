#ifndef FT_WALK_H
#define FT_WALK_H

#include <stdbool.h>

#include "./ft_ls.h"

bool walk(t_args *args, t_list *paths);
void free_path(void *content);

#endif // FT_WALK_H
