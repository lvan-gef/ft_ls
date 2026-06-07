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

#ifndef LOGIN_NAME_MAX
#define LOGIN_NAME_MAX INT64_C(256)
#endif // ifndef LOGIN_NAME_MAX //

#ifndef PATH_MAX
#define PATH_MAX INT64_C(4096)
#endif // ifndef PATH_MAX //

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
    t_str *symlink;
    const t_str *parent_path;
    t_file_info *info;
    struct stat st;
    uint64_t display_len;
    uint64_t padded_display_len;
    char quote;
    bool path_has_colon;
    bool is_operand;
    bool symlink_ready;
} t_entry;

t_entry *new_entry(const t_alloc *alloc, const t_entry *entry,
                   const struct dirent *dp);
bool get_file_info(const t_alloc *alloc, t_entry *entry);
void init_entry_display(t_entry *entry);
bool ensure_entry_path(const t_alloc *alloc, t_entry *entry);
t_entry *dup_dir_entry(const t_alloc *alloc, const t_entry *src,
                       bool is_operand);
t_str *read_symlink_target(const t_alloc *alloc, const t_str *path,
                           uint64_t target_size, int *read_err);
void free_entry(free_list *fl, t_entry *entry);

#endif // !FT_ENTRY_H
