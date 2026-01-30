#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_fprintf.h"
#include "../include/ft_ls.h"
#include "../include/ft_print.h"
#include "ft_arena.h"
#include "ft_parser.h"
#include "ft_sort.h"
#include "libft.h"

static char *walk_files_(Arena *arena, t_args *args, t_path *path, DIR *dir);
static bool print_(t_args *args, t_path *path, bool print_header);
static t_str *join_paths_(Arena *arena, t_str *path, t_str *filename);

void printer(t_args *args) {
    ASSERT_(args, "args can not be NULL");

    Arena *arena = args->paths->arena;
    char *err_msg = NULL;
    DIR *dir = NULL;

    t_array *queue = init_array(arena, DEFAULT_SIZE, ARRAY_PATHS);
    if (!queue) {
        ft_fprintf(STDERR_FILENO, "ft_ls: failed to allocate work queue\n");
        return;
    }

    size_t i = 0;
    while (i < args->paths->len) {
        if (!append_array(queue, args->paths->data[i])) {
            ft_fprintf(STDERR_FILENO, "ft_ls: failed to add path to queue\n");
            return;
        }
        ++i;
    }

    size_t queue_index = 0;
    while (queue_index < queue->len) {
        errno = 0;
        t_path *path = queue->data[queue_index];

        dir = opendir(path->name->str);
        if (!dir) {
            if (errno == EACCES) {
                ft_fprintf(STDERR_FILENO, "ft_ls: cannot access '%s': %s\n",
                           path->name->str, strerror(errno));
                ++queue_index;
                continue;
            }

            if (errno == ENOENT) {
                ft_fprintf(STDERR_FILENO, "ft_ls: cannot access '%s': %s\n",
                           path->name->str, strerror(errno));
                ++queue_index;
                continue;
            }

            ft_fprintf(STDERR_FILENO, "ft_ls: %s: %s\n", path->name->str,
                       strerror(errno));
            goto failed;
        }

        err_msg = walk_files_(arena, args, path, dir);
        closedir(dir);
        dir = NULL;

        if (err_msg || errno) {
            goto failed;
        }

        print_(args, path, args->recursive);

        if (args->recursive) {
            if (path->paths->len > 0) {
                sort_alpha(path->paths, args->reverse);
            }

            size_t sub_index = 0;
            while (sub_index < path->paths->len) {
                if (!append_array(queue, path->paths->data[sub_index])) {
                    err_msg = (char *)"failed to add subdirectory to queue";
                    goto failed;
                }
                ++sub_index;
            }
        }

        ++queue_index;
    }

    return;
failed:
    ft_fprintf(STDERR_FILENO, "errno: %d, %s\n", errno,
               err_msg ? err_msg : "unknown error");
    if (dir) {
        closedir(dir);
    }
}

static char *walk_files_(Arena *arena, t_args *args, t_path *path, DIR *dir) {
    ASSERT_(args, "args can not be NULL");
    ASSERT_(path, "path can not be NULL");
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

        t_file *file = init_file(arena);
        if (!file) {
            goto failed;
        }

        file->name = create_str(arena, dirent->d_name);
        if (!file->name) {
            goto failed;
        }

        file->type = dirent->d_type;
        if (file->type == DT_DIR && args->recursive) {
            if (ft_strncmp(file->name->str, ".", file->name->len + 1) &&
                ft_strncmp(file->name->str, "..", file->name->len + 1)) {

                t_str *fullname = join_paths_(arena, path->name, file->name);
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

static bool print_(t_args *args, t_path *path, bool print_header) {
    ASSERT_(args, "args can not be NULL");
    ASSERT_(path, "path can not be NULL");
    ASSERT_(path->files, "path->files can not be NULL");
    ASSERT_(path->files->len, "path->files->len must be > 0");

    if (print_header) {
        if (write(STDOUT_FILENO, "\n", 1) < 0) {
            return false;
        }

        if (write(STDOUT_FILENO, path->name->str, path->name->len) < 0) {
            return false;
        }

        if (write(STDOUT_FILENO, ":\n", 2) < 0) {
            return false;
        }
    }

    sort_alpha(path->files, args->reverse);

    size_t index = 0;
    while (index < path->files->len) {
        t_file *file = path->files->data[index];
        if (write(STDOUT_FILENO, file->name->str, file->name->len) < 0) {
            return false;
        }

        if (write(STDOUT_FILENO, "  ", 2) < 0) {
            return false;
        }

        ++index;
    }
    if (write(STDOUT_FILENO, "\n", 1) < 0) {
        return false;
    }

    return true;
}

static t_str *join_paths_(Arena *arena, t_str *path, t_str *filename) {
    ASSERT_(arena, "arena can not be NULL");
    ASSERT_(path, "path can not be NULL");
    ASSERT_(path->len, "path->len must be > 0");
    ASSERT_(*path->str, "*path->str can not be '\\0'");
    ASSERT_(filename, "filename can not be NULL)");
    ASSERT_(filename->len, "filename->len must be > 0");
    ASSERT_(*filename->str, "*filename->str can not be '\\0'");

    const U64 arena_pos = ArenaPos(arena);
    t_str *new_path = ArenaPush(arena, sizeof(*new_path));
    if (!new_path) {
        return NULL;
    }

    const size_t new_size = path->len + 1 + filename->len + 1;
    new_path->str = ArenaPush(arena, new_size);
    if (!new_path->str) {
        ArenaPopTo(arena, arena_pos);
        return NULL;
    }

    size_t len = ft_strlcpy(new_path->str, path->str, new_size);

    ASSERT_(len - 1 < len, "len did underflow");
    if (new_path->str[len - 1] != '/') {
        len += ft_strlcpy(new_path->str + len, "/", new_size - len);
    }
    len += ft_strlcpy(new_path->str + len, filename->str, new_size - len);

    new_path->len = len;
    new_path->cap = new_size - 1;

    return new_path;
}

// ./ft_ls -R /home/lvan-gef  0,42s user 0,18s system 99% cpu 0,601 total
// \    ls -R /home/lvan-gef  0,05s user 0,06s system 98% cpu 0,111 total
