#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../include/ft_arena.h"
#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_helpers.h"
#include "../include/ft_ls.h"
#include "../include/ft_print.h"
#include "../include/ft_sort.h"

#include "../libft/include/ft_fprintf.h"
#include "../libft/include/libft.h"

static void printer_(Arena *arena, const t_args *args, t_path *path);
static void print_row(Arena *arena, t_path *path);
static void print_list_(Arena *arena, t_path *path);
static bool calc_cols_(Arena *arena, t_path *path, size_t **col_widths,
                       size_t *num_cols, size_t *num_rows);
static size_t list_str_len_(t_path *path, size_t **lens, size_t *total);
static char *rigth_pad(Arena *arena, size_t nbr, size_t nbr_len);
#if defined(__linux__)
static size_t calc_layout_width_(t_array *files, size_t num_cols,
                                 size_t *col_widths, bool quoted);
#endif

void print_ls(t_args *args) {
    ASSERT_(args, "args can not be NULL");
    ASSERT_(args->paths, "args->paths can not be NULL");
    ASSERT_(args->paths->data, "args->path->data can not be NULL");
    ASSERT_(args->paths->data[0], "args->path->data[0] can not be NULL");

    t_array *paths = args->paths;
    if (!paths->len) {
        return;
    }

    Arena *arena = ArenaAlloc(4096);
    size_t index = 0;

    if (args->recursive) {
        if (args->time) {
            sort_time_files(args->paths, args->reverse);
        } else {
            sort_alpha_paths(args->paths, args->reverse);
        }
        while (index < paths->len) {
            t_path *path = paths->data[index];
            U64 arena_pos = ArenaPos(arena);
            const size_t str_len = ft_strlen(path->name) + 3;
            char *dir_nam = ArenaPush(arena, str_len);
            if (!dir_nam) {
                goto failed;
            }

            size_t len = ft_strlcpy(dir_nam, path->name, str_len);
            len += ft_strlcpy(dir_nam + len, ":\n", str_len);
            write(STDOUT_FILENO, dir_nam, len);
            ArenaPopTo(arena, arena_pos);

            printer_(arena, args, path);
            if (index + 1 < paths->len) {
                write(STDOUT_FILENO, "\n", 1);
            }
            ++index;
        }
    } else {
        while (index < paths->len) {
            t_path *path = paths->data[index];
            printer_(arena, args, path);
            ++index;
        }
    }

    ArenaRelease(arena);
    return;

failed:
    ArenaRelease(arena);
}

static void printer_(Arena *arena, const t_args *args, t_path *path) {
    ASSERT_(args, "args can not be NULL");
    ASSERT_(path, "path can not be NULL");
    ASSERT_(path->files, "path->files can not be NULL");
    ASSERT_(path->files->data, "path->files->data can not be NULL");
    ASSERT_(path->name, "path->path can not be NULL");
    ASSERT_(*path->name, "*path->path can not be '\\0'");

    if (!path->max_len) {
        if (args->list) {
            write(STDOUT_FILENO, "totaal 0\n", 9);
        }
        return;
    }

    if (args->time) {
        sort_time_files(path->files, args->reverse);
    } else {
        sort_alpha(path->files, args->reverse);
    }

    if (args->list) {
        print_list_(arena, path);
    } else {
        print_row(arena, path);
    }
}

static void print_row(Arena *arena, t_path *path) {
    ASSERT_(arena, "arena can not be NULL");
    ASSERT_(path, "path can not be NULL");
    ASSERT_(path->files, "path->files can not be NULL");
    ASSERT_(path->files->len, "path->files->len must be > 0");
    ASSERT_(path->files->data, "path->files->data can not be NULL");
    ASSERT_(path->files->data[0], "path->files->data[0] can not be NULL");
    ASSERT_(path->max_len, "path->max_len must be > 0");

    size_t num_cols = 1;
    size_t num_rows = path->files->len;
    size_t *col_widths = NULL;

    if (!calc_cols_(arena, path, &col_widths, &num_cols, &num_rows)) {
        ft_fprintf(STDERR_FILENO, "Failed to alloc memory in arena\n");
        return;
    }

    size_t *col_starts = ArenaPush(arena, (num_cols + 1) * sizeof(*col_starts));
    if (!col_starts) {
        ft_fprintf(STDERR_FILENO, "Failed to alloc memory in arena\n");
        return;
    }

    col_starts[0] = 0;
    for (size_t c = 0; c < num_cols; ++c) {
        col_starts[c + 1] = col_starts[c] + col_widths[c] + 2;
    }

    size_t buf_size = TERM_SIZE + 16;
    char *buf = ArenaPush(arena, buf_size);
    if (!buf) {
        ft_fprintf(STDERR_FILENO, "Failed to alloc memory in arena\n");
        return;
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

            if (path->quoted &&
                !(f->filename[0] == '"' || f->filename[0] == '\'')) {
                buf[buf_len] = ' ';
                ++buf_len;
                ++cur_pos;
            }

            buf_len +=
                ft_strlcpy(buf + buf_len, f->filename, buf_size - buf_len);
            cur_pos += f->filename_len;

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
}

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
#if defined(__linux__)
    size_t max_cols = path->files->len;
    if (max_cols > TERM_SIZE / 2) {
        max_cols = TERM_SIZE / 2;
    }

    *col_widths = ArenaPush(arena, max_cols * sizeof(**col_widths));
    if (!*col_widths) {
        return false;
    }

    for (size_t try_cols = max_cols; try_cols > 1; --try_cols) {
        size_t width = calc_layout_width_(path->files, try_cols, *col_widths,
                                          path->quoted);
        if (width < TERM_SIZE) {
            *num_cols = try_cols;
            *num_rows = (files_len + *num_cols - 1) / *num_cols;
            break;
        }
    }

    (void)calc_layout_width_(path->files, *num_cols, *col_widths, path->quoted);
#elif defined(__APPLE__)
    size_t colwidth = path->max_len;
    if (path->quoted) {
        colwidth += 1;
    }
    colwidth = (colwidth + 8) & ~((size_t)7);

    *num_cols = TERM_SIZE / colwidth;
    if (*num_cols < 1) {
        *num_cols = 1;
    }
    if (*num_cols > files_len) {
        *num_cols = files_len;
    }
    *num_rows = (files_len + *num_cols - 1) / *num_cols;

    *col_widths = ArenaPush(arena, *num_cols * sizeof(**col_widths));
    if (!*col_widths) {
        return false;
    }

    for (size_t c = 0; c < *num_cols; ++c) {
        (*col_widths)[c] = colwidth - 2;
    }
#else
    ft_fprintf(STDERR_FILENO, "OS is not supported\n");
    return false;
#endif

    return true;
}

#if defined(__linux__)
static size_t calc_layout_width_(t_array *files, size_t num_cols,
                                 size_t *col_widths, bool quoted) {
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
            ASSERT_(f->filename_len, "f->len can not be NULL");
            size_t len = f->filename_len;
            if (quoted && !(f->filename[0] == '"' || f->filename[0] == '\'')) {
                ASSERT_(len + 1 > len, "len did overflow");
                len += 1;
            }

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

static void print_list_(Arena *arena, t_path *path) {
    ASSERT_(arena, "arena can not be NULL");
    ASSERT_(path, "path can not be NULL");
    ASSERT_(path->files, "path->files can not be NULL");
    ASSERT_(path->files->len, "path->files->len must be > 0");
    ASSERT_(path->files->data, "path->files->data can not be NULL");
    ASSERT_(path->files->data[0], "path->files->data[0] can not be NULL");
    ASSERT_(path->max_len, "path->max_len must be > 0");

    size_t *lens = ArenaPush(arena, 8 * sizeof(*lens));
    if (!lens) {
        // TODO: print error
        return;
    }

    size_t total = 0;
    size_t char_count = list_str_len_(path, &lens, &total);
    if (!char_count) {
        return;
    }

    size_t str_len = char_count * sizeof(char);
    Arena *scratch_arena = ArenaAlloc(str_len);
    if (!scratch_arena) {
        return;
    }

    char *total_str = ArenaPush(scratch_arena, str_len);
    if (!total_str) {
        goto failed;
    }

    size_t len = ft_strlcpy(total_str, "totaal ", str_len);
    len += uitoa(total_str + len, str_len, total / 2);
    len += ft_strlcpy(total_str + len, "\n", str_len);
    if (write(STDOUT_FILENO, total_str, len) < 0) {
        goto failed;
    }
    ArenaClear(scratch_arena);

    size_t index = 0;
    while (index < path->files->len) {
        t_file *file = path->files->data[index];
        len = ft_strlcpy(total_str, file->permission, str_len);
        len += ft_strlcpy(total_str + len, " ", str_len);

        char *hardlink = rigth_pad(arena, file->hardlink, lens[1]);
        if (!hardlink) {
            goto failed;
        }
        len += ft_strlcpy(total_str + len, hardlink, str_len);
        len += ft_strlcpy(total_str + len, " ", str_len);

        size_t group_len = ft_strlcpy(total_str + len, file->group, str_len);
        len += group_len;
        while (group_len < lens[2]) {
            len += ft_strlcpy(total_str + len, " ", str_len);
            ++group_len;
        }
        len += ft_strlcpy(total_str + len, " ", str_len);

        size_t user_len = ft_strlcpy(total_str + len, file->user, str_len);
        len += user_len;
        while (user_len < lens[3]) {
            len += ft_strlcpy(total_str + len, " ", str_len);
            ++user_len;
        }
        len += ft_strlcpy(total_str + len, " ", str_len);

        char *size = rigth_pad(arena, (size_t)file->size, lens[4]);
        if (!size) {
            goto failed;
        }
        len += ft_strlcpy(total_str + len, size, str_len);
        len += ft_strlcpy(total_str + len, " ", str_len);

        len += ft_strlcpy(total_str + len, file->date_fmt, str_len);
        len += ft_strlcpy(total_str + len, " ", str_len);

        if (file->filename[0] == '\'' || file->filename[0] == '"') {
            len += ft_strlcpy(total_str + len, " ", str_len);
        }
        len += ft_strlcpy(total_str + len, file->filename, str_len);
        if (*file->linkedname) {
            len += ft_strlcpy(total_str + len, " -> ", str_len);
            len += ft_strlcpy(total_str + len, file->linkedname, str_len);
        }

        len += ft_strlcpy(total_str + len, "\n", str_len);
        if (write(STDOUT_FILENO, total_str, len) < 0) {
            goto failed;
        }

        ++index;
        ArenaClear(scratch_arena);
    }

    ArenaRelease(scratch_arena);
    return;
failed:
    // TODO: print error
    ArenaRelease(scratch_arena);
}

static size_t list_str_len_(t_path *path, size_t **lens, size_t *total) {
    size_t index = 0;
    size_t char_len = 1024 * sizeof(char);
    Arena *scratch_arena = ArenaAlloc(char_len * 2);
    while (index < path->files->len) {
        t_file *file = path->files->data[index];
        size_t permission_len = ft_strlen(file->permission);
        if (permission_len > (*lens)[0]) {
            (*lens)[0] = permission_len;
        }

        char *str = ArenaPush(scratch_arena, char_len);
        if (!str) {
            goto failed;
            return 0;
        }

        size_t hardlink_len = uitoa(str, char_len, file->hardlink);
        if (hardlink_len > (*lens)[1]) {
            (*lens)[1] = hardlink_len;
        }

        size_t group_len = ft_strlen(file->group);
        if (group_len > (*lens)[2]) {
            (*lens)[2] = group_len;
        }

        size_t user_len = ft_strlen(file->user);
        if (user_len > (*lens)[3]) {
            (*lens)[3] = user_len;
        }

        str = ArenaPush(scratch_arena, char_len);
        if (!str) {
            goto failed;
        }

        size_t size_len = uitoa(str, char_len, (size_t)file->size);
        if (size_len > (*lens)[4]) {
            (*lens)[4] = size_len;
        }
        ASSERT_(*total <= SIZE_MAX - file->blocks, "total did overflow");
        *total += file->blocks;

        size_t date_len = ft_strlen(file->date_fmt);
        if (date_len > (*lens)[5]) {
            (*lens)[5] = date_len;
        }

        size_t filename_len = ft_strlen(file->filename);
        if (filename_len > (*lens)[6]) {
            (*lens)[6] = filename_len;
        }

        if (*file->linkedname) {
            size_t extra_space = 4; // ' -> '
            size_t linked_len = ft_strlen(file->linkedname) + extra_space;
            if (linked_len > (*lens)[7]) {
                (*lens)[7] = linked_len;
            }
        }

        ++index;
        ArenaClear(scratch_arena);
    }

    size_t str_len = 0;
    index = 0;

    while (index < 8) {
        str_len += (*lens)[index] + 2;
        ++index;
    }

    ArenaRelease(scratch_arena);
    return str_len;
failed:
    // TODO: print error
    ArenaRelease(scratch_arena);
    return 0;
}

static char *rigth_pad(Arena *arena, size_t nbr, size_t nbr_len) {
    char *nbr_str = ArenaPush(arena, nbr_len + 1);
    if (!nbr_str) {
        return NULL;
    }

    size_t str_len = uitoa(nbr_str, nbr_len + 1, nbr);
    U64 arena_pos = ArenaPos(arena);
    char *buffer = ArenaPush(arena, nbr_len + 1);
    if (!buffer) {
        ArenaPopTo(arena, arena_pos);
        return NULL;
    }

    size_t pad_count = (nbr_len > str_len) ? nbr_len - str_len : 0;
    size_t len = 0;
    while (len < pad_count) {
        buffer[len] = ' ';
        ++len;
    }

    ft_strlcpy(buffer + len, nbr_str, nbr_len + 1 - len);
    return buffer;
}
