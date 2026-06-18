#ifndef FT_ENTRY_H
#define FT_ENTRY_H

#include <stdbool.h>
#include <sys/stat.h>

#include "./ft_str.h"

typedef struct {
    t_str *name;
    t_str *path;
    struct stat st;
    bool stat_unavailable;
    bool is_operand;
} t_entry;

#endif // !FT_ENTRY_H
