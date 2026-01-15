#ifndef FT_LS_H
#define FT_LS_H

typedef enum e_os {
    OS_LINUX,
    OS_MAC
} t_os;

#ifdef __linux__
    #include <linux/limits.h>
    #define CURRENT_OS ((t_os)OS_LINUX)
#elif defined(__APPLE__)
    #include <sys/syslimits.h>
    #define CURRENT_OS ((t_os)OS_MAC)
#endif

#include <stdbool.h>
#include <stddef.h>

#include "./ft_array.h"

#ifndef PERMISSION_SIZE
#define PERMISSION_SIZE 12
#endif // !PERMISSION_SIZE

#ifndef DT_LEN
#define DT_LEN 13
#endif // !DT_LEN

#ifndef TERM_SIZE
#define TERM_SIZE 80
#endif // !TERM_SIZE

#ifndef USER_SIZE
#define USER_SIZE 256
#endif // !USER_SIZE

typedef struct s_args {
    bool list;
    bool recursive;
    bool all;
    bool reverse;
    bool time;
    t_array *paths;
} t_args;

typedef struct s_path {
    size_t max_len;
    char path[PATH_MAX];
    bool quoted;
    t_array *files;
} t_path;

typedef struct s_file {
    unsigned long hardlink;
    long long size;
    size_t len;
    char filename[NAME_MAX];
    char permission[PERMISSION_SIZE];
    char group[USER_SIZE];
    char user[USER_SIZE];
    char date_fmt[DT_LEN];
    // add time as number
} t_file;

#endif // !FT_LS_H
