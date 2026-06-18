#ifndef FT_WALK_INTERNAL_H
#define FT_WALK_INTERNAL_H

#include <dirent.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/stat.h>

#include "../include/ft_entry.h"
#include "../include/ft_str.h"

#include "./ft_arena.h"

t_entry *walk_entry_new_file_operand(const t_str *path, const struct stat *st);
t_entry *walk_entry_new_owned_path(const t_str *path, const struct stat *st,
                                   bool is_operand);
t_entry *walk_entry_new_scratch_dirent(Arena *scratch, const struct dirent *dp);
bool walk_entry_build_path(Arena *scratch, t_entry *entry,
                           const t_str *parent_path);
void walk_entry_free(t_entry *entry);
void walk_entry_del(void *ptr);

bool walk_path_print_error(t_str *out, const t_str *path, int e,
                           const char *prefix, bool *output_failed);
mode_t walk_dtype_to_mode(unsigned char dtype);
bool walk_dtype_needs_lstat(bool list, bool time, unsigned char dtype);

#endif // !FT_WALK_INTERNAL_H
