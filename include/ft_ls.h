#ifndef FT_LS_H
#define FT_LS_H

#include "ft_arena.h"
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

#ifndef ARENA_SIZE
#define ARENA_SIZE 4096
#endif // !ARENA_SIZE

typedef struct {
    bool list;
    bool recursive;
    bool all;
    bool reverse;
    bool time;
    t_array *paths;
} t_args;

typedef struct {
    char *str;
    size_t cap;
    size_t len;
} t_str;

typedef struct {
    size_t max_len;
    t_str *name;
    t_array *files;
    t_array *paths;
} t_path;

typedef struct {
    t_str *name;
    unsigned char type;
} t_file;

t_path *init_path(Arena *arena);
t_file *init_file(Arena *arena);
t_str *create_str(Arena *arena, const char *str);

#endif // !FT_LS_H
