#ifndef FT_GET_STATS_H
#define FT_GET_STATS_H

#include <stdbool.h>
#include <sys/stat.h>

#include "./ft_arena.h"
#include "./ft_ls.h"

bool get_stat(struct stat *sb, const char *fullpath);
bool get_permission(Arena *arena, struct stat *sb, t_file *file);
bool get_hardlink(Arena *arena, struct stat *sb, t_file *file);
bool get_user(Arena *arena, t_file *file, uid_t user_id);
bool get_group(Arena *arena, t_file *file, gid_t group_id);
bool get_size(Arena *arena, struct stat *sb, t_file *file);
bool get_dt(Arena *arena, struct stat *sb, t_file *file);
bool get_linked_name(Arena *arena, struct stat *sb, t_file *file, const char *fullname);

#endif // !FT_GET_STATS_H
