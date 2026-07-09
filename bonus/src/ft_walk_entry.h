#ifndef FT_WALK_ENTRY_H
#define FT_WALK_ENTRY_H

#include <dirent.h>
#include <stdbool.h>

#include "../include/ft_str.h"

#include "./ft_arena.h"
#include "./ft_ls.h"

t_entry *entry_new_file_operand(const t_str *path, const struct stat *st);
t_entry *entry_new_path(const t_str *path, const struct stat *st,
                        bool is_operand);
t_entry *entry_new_dirent(Arena *scratch, const struct dirent *dp);
bool entry_build_path(Arena *scratch, t_entry *entry, const t_str *parent_path);
void entry_free(t_entry *entry);
void entry_del(void *ptr);

#endif /* ifndef FT_WALK_ENTRY_H */
