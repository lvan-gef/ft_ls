#ifndef FT_WALK_H
#define FT_WALK_H

#include <stdbool.h>

#include "../libft/include/libft.h"
#include "./ft_ls.h"

bool walk(t_args *args, t_list **paths);
void free_walk(void *content);

#endif // FT_WALK_H
