#ifndef FT_PATH_H
#define FT_PATH_H

#include <sys/stat.h>

#include "ft_str.h"

typedef struct {
    t_str *name;
    t_str *path;
    struct stat st;
} t_entry;

#endif // !FT_PATH_H
