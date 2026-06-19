#ifndef FT_ENTRY_H
#define FT_ENTRY_H

#include <stdbool.h>
#include <sys/stat.h>

#include "./ft_shell_scan.h"

typedef struct s_str t_str;

typedef struct s_entry {
    t_str *name;
    t_str *path;
    t_shell_scan name_scan;
    struct stat st;
    bool stat_unavailable;
    bool is_operand;
} t_entry;

#endif // !FT_ENTRY_H
