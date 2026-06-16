#ifndef FT_ENTRY_H
#define FT_ENTRY_H

#include <dirent.h>
#include <limits.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/stat.h>

#include "./ft_free_list.h"
#include "./ft_str.h"
#include "./ft_arena.h"

typedef struct {
    t_str *perm;
    t_str *links;
    t_str *username;
    t_str *groupname;
    t_str *size;
    t_str *symlink;
    t_str *dt;
    uint64_t blocks;
} t_file_info;

typedef struct {
    t_str *name;
    t_str *path;
    const t_str *parent_path;
    t_file_info *info;
    struct stat st;
    bool stat_unavailable;
    bool is_operand;
} t_entry;

t_entry *new_scanned_entry(Arena *arena, const t_entry *parent,
                           const struct dirent *dp);
bool fill_file_info(Arena *arena, t_entry *entry);
bool ensure_entry_path(Arena *arena, t_entry *entry);

t_entry *dup_dir_entry(free_list *fl, const t_entry *src, bool is_operand);
t_str *read_symlink_target(Arena *arena, const t_str *path,
                           uint64_t target_size, int *read_err);
void free_entry(free_list *fl, const t_entry *entry);

#endif // !FT_ENTRY_H
