#ifndef FT_ENTRY_H
#define FT_ENTRY_H

#include <dirent.h>
#include <limits.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/stat.h>

#include "./ft_str.h"
#include "ft_free_list.h"

#ifndef LOGIN_NAME_MAX
#define LOGIN_NAME_MAX INT64_C(256)
#endif // ifndef LOGIN_NAME_MAX //

#ifndef PATH_MAX
#define PATH_MAX INT64_C(4096)
#endif // ifndef PATH_MAX //

#ifndef PERMISSION_SIZE
#define PERMISSION_SIZE UINT64_C(12)
#endif // ifndef PERMISSION_SIZE //

#ifndef DT_LEN
#define DT_LEN UINT64_C(13)
#endif // ifndef DT_LEN //

#ifndef ROW_ELEMENTS
#define ROW_ELEMENTS UINT64_C(8)
#endif // ifndef ROW_ELEMENTS //

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
    char quote;
    bool is_operand;
    t_file_info *info;
    struct stat st;
    bool is_escaped;
    uint64_t display_len;
    uint64_t padded_display_len;
} t_entry;

t_entry *new_entry(Arena *arena, t_entry *entry, const struct dirent *dp);
bool get_file_info(free_list *fl, t_entry *entry);
bool get_file_info_arena(Arena *arena, t_entry *entry);
void init_entry_display(t_entry *entry);
void free_entry(free_list *fl, t_entry *entry);

#endif // !FT_ENTRY_H
