#ifndef FT_WALK_ENTRY_H
#define FT_WALK_ENTRY_H

#include <stdbool.h>

#include "../include/ft_str.h"

#include "./ft_arena.h"
#include "./ft_ls.h"

struct dirent;
struct stat;

typedef struct s_arena Arena;
typedef struct s_entry t_entry;
typedef struct s_str t_str;

t_entry *entry_new_file_operand(const t_str *path, const struct stat *st);
t_entry *entry_new_path(const t_str *path, const struct stat *st,
                        bool is_operand);
t_entry *entry_new_dirent(Arena *scratch, const struct dirent *dp);
bool entry_build_path(Arena *scratch, t_entry *entry, const t_str *parent_path);
void entry_free(t_entry *entry);
void entry_del(void *ptr);

#endif /* ifndef FT_WALK_ENTRY_H */
