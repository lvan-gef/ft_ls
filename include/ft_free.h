#ifndef FT_FREE_H
#define FT_FREE_H

#include "./ft_ls.h"

void free_args(t_args *args);
void free_walk(void *content);
void free_file(void *content);
void free_path(void *content);

#endif // !FT_FREE_H
