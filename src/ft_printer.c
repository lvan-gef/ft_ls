#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../include/ft_arena.h"
#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_fprintf.h"
#include "../include/ft_ls.h"
#include "../include/ft_parser.h"
#include "../include/ft_print.h"
#include "../include/ft_sort.h"
#include "../libft/include/libft.h"

#if defined(__APPLE__)
#include "../include/ft_printer_mac.h"
#endif

static int open_dir_(Arena *file_arena, DIR **dir, t_path *path);
static char *walk_files_(Arena *arena, Arena *file_arena, t_args *args,
                         t_path *path, DIR *dir);
static char *append_queue(t_args *args, t_array *queue, size_t queue_index,
                          t_array *paths);
static bool print_(t_args *args, t_path *path, bool print_header, size_t queue_index);
static t_str *join_paths_(Arena *arena, t_str *path, t_str *filename);

#if defined(__linux__)
static bool calc_cols_(Arena *arena, t_path *path, size_t **col_widths,
                       size_t *num_cols, size_t *num_rows);
static size_t calc_layout_width_(t_array *files, size_t num_cols,
                                 size_t *col_widths);
#endif

void printer(t_args *args) {
    ASSERT_(args, "args can not be NULL");

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
    ASSERT_(args, "args can not be NULL");
    ASSERT_(path, "path can not be NULL");
    ASSERT_(file_arena, "file_arena can not be NULL");
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

static char *append_queue(t_args *args, t_array *queue, size_t queue_index,
                          t_array *paths) {
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

static bool print_(t_args *args, t_path *path, bool print_header, size_t queue_index) {
    ASSERT_(args, "args can not be NULL");
    ASSERT_(path, "path can not be NULL");
    ASSERT_(path->files, "path->files can not be NULL");

    t_map map = {.col = 1, .row = path->files->len};

#if defined(__APPLE__)
    return print_mac(args, path, &map, print_header, queue_index);
#elif defined(__linux__)
    if (print_header) {
        if (write(STDOUT_FILENO, path->name->str, path->name->len) < 0) {
            return false;
        }

        if (write(STDOUT_FILENO, ":\n", 2) < 0) {
            return false;
        }
    }

    if (!path->files->len) {
        return true;
    }

    if (args->time) {

    } else {
        sort_alpha(path->files, args->reverse);
    }
    size_t *col_widths = NULL;

    if (!calc_cols_(path->paths->arena, path, &col_widths, &num_cols,
                    &num_rows)) {
        return false;
    }

    size_t *col_starts =
        ArenaPush(path->paths->arena, (num_cols + 1) * sizeof(*col_starts));
    if (!col_starts) {
        return false;
    }

    col_starts[0] = 0;
    for (size_t c = 0; c < num_cols; ++c) {
        col_starts[c + 1] = col_starts[c] + col_widths[c] + 2;
    }

    size_t buf_size = TERM_SIZE + 16;
    char *buf = ArenaPush(path->paths->arena, buf_size);
    if (!buf) {
        ft_fprintf(STDERR_FILENO, "Failed to alloc memory in arena\n");
        return false;
    }

    const size_t files_len = path->files->len;
    for (size_t row = 0; row < num_rows; ++row) {
        size_t buf_len = 0;
        size_t cur_pos = 0;

        for (size_t col = 0; col < num_cols; ++col) {
            size_t idx = row + col * num_rows;
            if (idx >= files_len) {
                break;
            }

            const t_file *f = path->files->data[idx];
            bool is_last_col = (col == num_cols - 1) ||
                               (row + (col + 1) * num_rows >= files_len);

            // if (path->quoted &&
            //     !(f->filename[0] == '"' || f->filename[0] == '\'')) {
            //     buf[buf_len] = ' ';
            //     ++buf_len;
            //     ++cur_pos;
            // }

            buf_len +=
                ft_strlcpy(buf + buf_len, f->name->str, buf_size - buf_len);
            cur_pos += f->name->len;

            if (!is_last_col) {
                size_t target_pos = col_starts[col + 1];
                size_t gap = target_pos - cur_pos;

                size_t test_pos = cur_pos;
                size_t num_tabs = 0;
                while (test_pos < target_pos) {
                    size_t next_tab = ((test_pos / 8) + 1) * 8;
                    if (next_tab > target_pos) {
                        break;
                    }

                    test_pos = next_tab;
                    ++num_tabs;
                }

                size_t spaces_after_tabs = target_pos - test_pos;
                size_t chars_with_tabs = num_tabs + spaces_after_tabs;
                if (num_tabs > 0 && chars_with_tabs < gap) {
                    while (cur_pos < target_pos) {
                        size_t next_tab = ((cur_pos / 8) + 1) * 8;
                        if (next_tab <= target_pos) {
                            buf[buf_len] = '\t';
                            ++buf_len;
                            cur_pos = next_tab;
                        } else {
                            buf[buf_len] = ' ';
                            ++buf_len;
                            ++cur_pos;
                        }
                    }
                } else {
                    while (cur_pos < target_pos) {
                        buf[buf_len] = ' ';
                        ++buf_len;
                        ++cur_pos;
                    }
                }
            }
        }

        buf[buf_len] = '\n';
        ++buf_len;
        if (write(STDOUT_FILENO, buf, buf_len) < 0) {
            break;
        }
    }
#endif
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

#if defined(__linux__)
static bool calc_cols_(Arena *arena, t_path *path, size_t **col_widths,
                       size_t *num_cols, size_t *num_rows) {
    ASSERT_(arena, "arena con not be NULL");
    ASSERT_(path, "path can not be NULL");
    ASSERT_(path->files, "path->files can not be NULL");
    ASSERT_(path->files->len, "path->files->len must be > 0");
    ASSERT_(path->files->data, "path->files->data can not be NULL");
    ASSERT_(path->files->data[0], "path->files->data[0] can not be NULL");
    ASSERT_(col_widths, "col_widths can not be NULL");
    ASSERT_(num_cols, "num_cols can not be NULL");
    ASSERT_(*num_cols, "*num_cols must be > 0");
    ASSERT_(num_rows, "num_rows can not be NULL");
    ASSERT_(*num_rows, "*num_rows must be > 0");

    const size_t files_len = path->files->len;
    size_t max_cols = path->files->len;
    if (max_cols > TERM_SIZE / 2) {
        max_cols = TERM_SIZE / 2;
    }

    *col_widths = ArenaPush(arena, max_cols * sizeof(**col_widths));
    if (!*col_widths) {
        return false;
    }

    for (size_t try_cols = max_cols; try_cols > 1; --try_cols) {
        size_t width = calc_layout_width_(path->files, try_cols, *col_widths);
        if (width < TERM_SIZE) {
            *num_cols = try_cols;
            *num_rows = (files_len + *num_cols - 1) / *num_cols;
            break;
        }
    }

    (void)calc_layout_width_(path->files, *num_cols, *col_widths);

    return true;
}

static size_t calc_layout_width_(t_array *files, size_t num_cols,
                                 size_t *col_widths) {
    ASSERT_(files, "files can not be NULL");
    ASSERT_(files->len, "files->len must be > 0");
    ASSERT_(files->data, "files->data can not be NULL");
    ASSERT_(files->data[0], "files->data[0] can not be NULL");
    ASSERT_(num_cols, "num_cols musr be > 0");
    ASSERT_(col_widths, "col_widths can not be NULL");

    size_t num_rows = (files->len + num_cols - 1) / num_cols;
    for (size_t c = 0; c < num_cols; ++c) {
        col_widths[c] = 0;
    }

    for (size_t col = 0; col < num_cols; ++col) {
        for (size_t row = 0; row < num_rows; ++row) {
            ASSERT_(row + col * num_rows >= row + col, "index did overflow");
            size_t index = row + col * num_rows;
            if (index >= files->len) {
                break;
            }

            const t_file *f = files->data[index];
            ASSERT_(f, "f can not be NULL");
            ASSERT_(f->name->str, "f->len can not be NULL");
            size_t len = f->name->len;
            // if (quoted && !(f->filename[0] == '"' || f->filename[0] == '\''))
            // {
            //     ASSERT_(len + 1 > len, "len did overflow");
            //     len += 1;
            // }

            if (len > col_widths[col]) {
                col_widths[col] = len;
            }
        }
    }

    size_t total = 0;
    for (size_t c = 0; c < num_cols; ++c) {
        total += col_widths[c];
        if (c < num_cols - 1) {
            ASSERT_(total + 2 > total, "total did overflow");
            total += 2;
        }
    }

    return total;
}
#endif
