#ifndef FT_FILE_INFO_H
#define FT_FILE_INFO_H

#include <stdbool.h>
#include <stdint.h>

#include "../include/ft_array.h"
#include "../include/ft_str.h"

#include "./ft_arena.h"

typedef struct s_file_info {
    t_str *perm;
    t_str *links;
    t_str *username;
    t_str *groupname;
    t_str *size;
    t_str *symlink;
    t_str *dt;
    t_str *major;
    t_str *minor;
    uint64_t blocks;
} t_file_info;

bool prepare_list_infos(t_arena *arena, const t_array *entries,
                        t_file_info **infos, bool access_time);

#endif /* ifndef FT_FILE_INFO_H */
