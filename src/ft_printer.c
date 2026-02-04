#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../include/ft_arena.h"
#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_helpers.h"
#include "../include/ft_ls.h"
#include "../include/ft_parser.h"
#include "../include/ft_print.h"
#include "../include/ft_print_list.h"
#include "../include/ft_sort.h"

#include "../include/ft_fprintf.h"
#include "../libft/include/libft.h"

static int open_dir_(Arena *file_arena, DIR **dir, t_path *path);
static char *walk_files_(Arena *arena, Arena *file_arena, t_args *args,
                         t_path *path, DIR *dir);
static char *append_queue(t_args *args, t_array *queue, size_t queue_index,
                          t_array *paths);
static bool print_(t_args *args, t_path *path, bool print_header,
                   size_t queue_index);

#if defined(__linux__)
#include "../include/ft_printer_linux.h"
#endif

#if defined(__APPLE__)
#include "../include/ft_printer_mac.h"
#endif

void printer(t_args *args) {
    ASSERT_(args, "args can not be NULL");
    ASSERT_(args->paths, "args->path can not be NULL");
    ASSERT_(args->paths->arena, "args->path->arena can not be NULL");
    ASSERT_(*args->paths->data, "*args->paths->data can not be NULL");
    ASSERT_(args->paths->len, "args->paths->len must be > 0");

    Arena *arena = args->paths->arena;
    char *err_msg = NULL;
    DIR *dir = NULL;

    Arena *file_arena = ArenaAlloc(ARENA_SIZE);
    if (!file_arena) {
        ft_fprintf(STDERR_FILENO, "ft_ls: failed to allocate file arena\n");
        return;
    }
    ArenaSetAutoAlign(file_arena, 8);

    t_array *queue = init_array(arena, DEFAULT_SIZE, ARRAY_PATHS);
    if (!queue) {
        ft_fprintf(STDERR_FILENO, "ft_ls: failed to allocate work queue\n");
        ArenaRelease(file_arena);
        return;
    }

    // err_msg = append_queue(args, queue, queue_index, args->paths);
    // if (err_msg) {
    //     goto failed;
    // }
    size_t i = 0;
    while (i < args->paths->len) {
        if (!append_array(queue, args->paths->data[i])) {
            ft_fprintf(STDERR_FILENO, "ft_ls: failed to add path to queue\n");
            ArenaRelease(file_arena);
            return;
        }
        ++i;
    }

    size_t queue_index = 0;
    while (queue_index < queue->len) {
        errno = 0;
        t_path *path = queue->data[queue_index];

        int result = open_dir_(file_arena, &dir, path);
        if (result < 0) {
            ++queue_index;
            continue;
        } else if (!result) {
            goto failed;
        }

        err_msg = walk_files_(arena, file_arena, args, path, dir);
        closedir(dir);
        dir = NULL;

        if (err_msg || errno) {
            goto failed;
        }

        if (queue_index) {
            write(STDOUT_FILENO, "\n", 1);
        }
        print_(args, path, args->recursive, queue_index);

        ArenaClear(file_arena);
        path->max_len = 0;

        err_msg = append_queue(args, queue, queue_index, path->paths);
        if (err_msg) {
            goto failed;
        }

        ++queue_index;
    }

    ArenaRelease(file_arena);
    return;
failed:
    ft_fprintf(STDERR_FILENO, "errno: %d, %s\n", errno,
               err_msg ? err_msg : "unknown error");
    if (dir) {
        closedir(dir);
    }

    ArenaRelease(file_arena);
}

static int open_dir_(Arena *file_arena, DIR **dir, t_path *path) {
    ASSERT_(file_arena, "file_arena can not be NULL");
    ASSERT_(dir, "can not be NULL");
    ASSERT_(path, "path can not be NULL");
    ASSERT_(path->name, "path->name can not be NULL");
    ASSERT_(*path->name->str, "*path->name->str can not be '\\0'");

    *dir = opendir(path->name->str);
    if (!*dir) {
        if (errno == EACCES || errno == ENOENT || errno == EPERM) {
            ft_fprintf(STDERR_FILENO, "ft_ls: cannot access '%s': %s\n",
                       path->name->str, strerror(errno));
            errno = 0;
            return -1;
        }

        ft_fprintf(STDERR_FILENO, "ft_ls: %s: %s\n", path->name->str,
                   strerror(errno));
        return 0;
    }

    path->files = init_array(file_arena, DEFAULT_SIZE, ARRAY_FILES);
    if (!path->files) {
        ft_fprintf(STDERR_FILENO, "ft_ls: failed to allocate files array\n");
        return 0;
    }

    return 1;
}

static char *walk_files_(Arena *arena, Arena *file_arena, t_args *args,
                         t_path *path, DIR *dir) {
    ASSERT_(arena, "arena can not be NULL");
    ASSERT_(file_arena, "file_arena can not be NULL");
    ASSERT_(args, "args can not be NULL");
    ASSERT_(path, "path can not be NULL");
    ASSERT_(path->name, "path->name can not be NULL");
    ASSERT_(*path->name->str, "*path->name->str can not be NULL");
    ASSERT_(dir, "dir can not be NULL");

    errno = 0;
    const struct dirent *dirent = readdir(dir);
    if (!dirent && errno) {
        return strerror(errno);
    }

    while (dirent) {
        errno = 0;

        if (*dirent->d_name == '.' && !args->all) {
            dirent = readdir(dir);
            continue;
        }

        t_file *file = init_file(file_arena);
        if (!file) {
            goto failed;
        }

        file->name = create_str(file_arena, dirent->d_name);
        if (!file->name) {
            goto failed;
        }

        file->type = dirent->d_type;
        if (file->type == DT_DIR && args->recursive) {
            if (ft_strncmp(file->name->str, ".", file->name->len + 1) &&
                ft_strncmp(file->name->str, "..", file->name->len + 1)) {

                t_str *fullname = join_paths(arena, path->name, file->name);
                if (!fullname) {
                    goto failed;
                }

                if (!add_path(path->paths, fullname->str)) {
                    goto failed;
                }
            }
        }

        if (file->name->len > path->max_len) {
            path->max_len = file->name->len;
        }

        if (!append_array(path->files, (void *)file)) {
            goto failed;
        }

        dirent = readdir(dir);
    }

    errno = 0;
    return NULL;

failed:
    return strerror(errno);
}

static char *append_queue(t_args *args, t_array *queue, size_t queue_index,
                          t_array *paths) {
    ASSERT_(args, "args can not be NULL");
    ASSERT_(queue, "queue can not be NULL");
    ASSERT_(paths, "paths can not be NULL");

    if (args->recursive) {
        if (args->time) {
        } else {
            sort_alpha(paths, args->reverse);
        }

        size_t index = 0;
        while (index < paths->len) {
            if (!insert_array(queue, queue_index + 1 + index,
                              paths->data[index])) {
                return (char *)"failed to add subdirectory to queue";
            }
            ++index;
        }
    }

    return NULL;
}

static bool print_(t_args *args, t_path *path, bool print_header,
                   size_t queue_index) {
    ASSERT_(args, "args can not be NULL");
    ASSERT_(path, "path can not be NULL");
    ASSERT_(path->files, "path->files can not be NULL");

    if (args->list) {
        if (args->time) {

        } else {
            sort_alpha(path->files, args->reverse);
        }
        return print_list(path, path->files);
    }

    t_map map = {.col = 1, .row = path->files->len};

#if defined(__APPLE__)
    return print_mac(args, path, &map, print_header, queue_index);
#elif defined(__linux__)
    (void)queue_index;
    return print_linux(args, path, &map, print_header);
#endif
    return false;
}
