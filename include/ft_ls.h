#ifndef FT_LS_H
#define FT_LS_H

#if defined(__linux__)
#include <linux/limits.h>
#else
#include <sys/syslimits.h>
#endif

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

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

#ifndef DEFAULT_SIZE
#define DEFAULT_SIZE 10
#endif // !DEFAULT_SIZE

typedef struct {
    bool list;
    bool recursive;
    bool all;
    bool reverse;
    bool time;
    t_array *paths;
} t_args;

typedef struct {
    size_t max_len;
    char name[PATH_MAX];
    bool quoted;
    struct timespec mtime;
    t_array *files;
} t_path;

typedef struct {
    unsigned long hardlink;
    unsigned long blocks;
    long long size;
    size_t filename_len;
    char filename[NAME_MAX];
    char linkedname[NAME_MAX];
    char permission[PERMISSION_SIZE];
    char group[USER_SIZE];
    char user[USER_SIZE];
    char date_fmt[DT_LEN];
    struct timespec mtime;
} t_file;

#endif // !FT_LS_H
