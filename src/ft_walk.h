#ifndef FT_WALK_H
#define FT_WALK_H

#include <dirent.h>
#include <stdbool.h>

#include "./ft_arena.h"
#include "./ft_array.h"
#include "./ft_ls.h"
#include "./ft_parse.h"
#include "./ft_str.h"

int process(const t_args *args, const t_array *array);

t_entry *entry_new_path(const t_str *path, const struct stat *st,
                        bool is_operand);
t_entry *entry_new_dirent(t_arena *scratch, const struct dirent *dp,
                          const t_str *dir_path);
void entry_free(t_entry *entry);
void entries_free(t_array *entries);

#endif /* ifndef FT_WALK_H */
