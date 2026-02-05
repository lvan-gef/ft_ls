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
#define DT_LEN 16
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

typedef enum {
    LIST_ENUM_PERMISSION,
    LIST_ENUM_HARDLINK,
    LIST_ENUM_USER,
    LIST_ENUM_GROUP,
    LIST_ENUM_SIZE,
    LIST_ENUM_DT,
    LIST_ENUM_NAME,
    LIST_ENUM_LINK,
    LIST_ENUM_COUNT
} e_list;

typedef struct {
    bool list;
    bool recursive;
    bool all;
    bool reverse;
    bool time;
    bool print_header;
    int exit_code;
    t_array *paths;
} t_args;

typedef struct {
    char *str;
    size_t cap;
    size_t len;
} t_str;

typedef struct {
    size_t max_len;
    struct timespec mtime;
    bool print_total;
    t_str *name;
    t_array *files;
    t_array *paths;
} t_path;

typedef struct {
    t_str *str;
    size_t count;
} t_hardlink;

typedef struct {
    t_str *str;
    size_t size;
} t_size;

typedef struct {
    unsigned char type;
    struct timespec mtime;
    size_t blocks;
    t_str *name;
    t_str *permission;
    t_str *user;
    t_str *group;
    t_str *dt;
    t_str *linked_name;
    t_hardlink *hardlink;
    t_size *size;
} t_file;

typedef struct {
    size_t col;
    size_t row;
} t_map;

t_path *init_path(Arena *arena);
t_file *init_file(Arena *arena);
t_str *create_str(Arena *arena, const char *str);

#endif // !FT_LS_H
