#ifndef FT_LS_H
#define FT_LS_H

#include <stdbool.h>

#include "../libft/include/libft.h"


#ifndef MAX_PATH
#define MAX_PATH 255
#endif // !MAX_PATH

#ifndef PERMISSION_SIZE
#define PERMISSION_SIZE 12
#endif // !PERMISSION_SIZE

#ifndef DT_LEN
#define DT_LEN 13
#endif // !DT_LEN

#ifndef TERM_SIZE
#define TERM_SIZE 80
#endif // !TERM_SIZE

typedef struct s_args {
    bool list;
    bool recursive;
    bool all;
    bool reverse;
    bool time;
    t_list *paths;
} t_args;

typedef struct s_path {
    char path[MAX_PATH];
    size_t max_len;
    size_t file_count;
    t_list *files;
} t_path;

typedef struct s_file {
    char filename[MAX_PATH];
    char permission[PERMISSION_SIZE];
    unsigned long hardlink;
    char group[MAX_PATH];
    char user[MAX_PATH];
    long long size;
    char date[DT_LEN];
    size_t len;
} t_file;

typedef struct s_node {
    char path[MAX_PATH];
} t_node;

#endif // FT_LS_H
