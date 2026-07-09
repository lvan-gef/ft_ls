#ifndef FT_LS_H
#define FT_LS_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/stat.h>

#include "../include/ft_str.h"

typedef struct s_shell_scan {
    uint64_t len;
    uint64_t display_len;
    uint64_t padded_display_len;
    char quote;
} t_shell_scan;

typedef struct s_entry {
    t_str *name;
    t_str *path;
    t_shell_scan name_scan;
    struct stat st;
    bool stat_unavailable;
    bool is_operand;
} t_entry;

#endif /* ifndef FT_LS_H */
