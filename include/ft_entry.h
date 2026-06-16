#ifndef FT_ENTRY_H
#define FT_ENTRY_H

#include <dirent.h>
#include <limits.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/stat.h>

#include "./ft_arena.h"
#include "./ft_free_list.h"
#include "./ft_str.h"

typedef struct {
    t_str *name;
    t_str *path;
    const t_str *parent_path;
    struct stat st;
    bool stat_unavailable;
    bool is_operand;
} t_entry;

t_entry *arena_new_entry(Arena *arena, const t_entry *parent,
                         const struct dirent *dp);
bool arena_entry_path(Arena *arena, t_entry *entry);
t_entry *fl_dup_entry(free_list *fl, const t_entry *src, bool is_operand);
t_str *arena_read_symlink(Arena *arena, const t_str *path, uint64_t target_size,
                          int *read_err);
void free_entry(free_list *fl, const t_entry *entry);

#endif // !FT_ENTRY_H
