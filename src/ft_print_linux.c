#include "ft_arena.h"
#include "ft_helpers.h"
#if defined(__linux__)

#include <stdbool.h>
#include <stddef.h>
#include <unistd.h>

#include "../include/ft_assert.h"
#include "../include/ft_ls.h"
#include "../include/ft_printer_linux.h"
#include "../include/ft_sort.h"
#include "ft_print_list.h"

#include "../libft/include/ft_fprintf.h"
#include "../libft/include/libft.h"

static bool calc_cols_(Arena *arena, t_path *path, size_t **col_widths,
                       t_map *map);
static size_t calc_layout_width_(t_array *files, size_t num_cols,
                                 size_t *col_widths);

bool print_linux(t_args *args, t_path *path, t_map *map, bool print_header) {
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
    if (!calc_cols_(path->paths->arena, path, &col_widths, map)) {
        return false;
    }

    size_t *col_starts =
        ArenaPush(path->paths->arena, (map->col + 1) * sizeof(*col_starts));
    if (!col_starts) {
        return false;
    }

    col_starts[0] = 0;
    for (size_t c = 0; c < map->col; ++c) {
        col_starts[c + 1] = col_starts[c] + col_widths[c] + 2;
    }

    size_t buf_size = TERM_SIZE + 16;
    char *buf = ArenaPush(path->paths->arena, buf_size);
    if (!buf) {
        ft_fprintf(STDERR_FILENO, "Failed to alloc memory in arena\n");
        return false;
    }

    const size_t files_len = path->files->len;
    for (size_t row = 0; row < map->row; ++row) {
        size_t buf_len = 0;
        size_t cur_pos = 0;

        for (size_t col = 0; col < map->col; ++col) {
            size_t idx = row + col * map->row;
            if (idx >= files_len) {
                break;
            }

            const t_file *f = path->files->data[idx];
            bool is_last_col = (col == map->col - 1) ||
                               (row + (col + 1) * map->row >= files_len);

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
    return true;
}

bool linux_list_format(Arena *arena, t_file_list *fl, t_file *file, char **output_str) {
    switch (fl->list_index) {
        case LIST_ENUM_PERMISSION:
            fl->wb_len += ft_strlcpy(*output_str + fl->wb_len, file->permission->str,
                                    fl->buffer_len);
            fl->wb_len +=
                ft_strlcpy(*output_str + fl->wb_len, " ", fl->buffer_len - fl->wb_len);
            break;
        case LIST_ENUM_HARDLINK: {
            char *hardlink = left_pad(arena, file->hardlink->count, fl->lens[LIST_ENUM_HARDLINK]);
            if (!hardlink) {
                return false;
            }

            fl->wb_len +=
                ft_strlcpy(*output_str + fl->wb_len, hardlink, fl->buffer_len - fl->wb_len);
            fl->wb_len +=
                ft_strlcpy(*output_str + fl->wb_len, " ", fl->buffer_len - fl->wb_len);
            break;
        }
        case LIST_ENUM_USER: {
            char *user = rigth_pad(arena, file->user, fl->lens[LIST_ENUM_USER]);
            if (!user) {
                return false;
            }

            fl->wb_len +=
                ft_strlcpy(*output_str + fl->wb_len, user, fl->buffer_len - fl->wb_len);
            fl->wb_len +=
                ft_strlcpy(*output_str + fl->wb_len, " ", fl->buffer_len - fl->wb_len);
            break;
        }
        case LIST_ENUM_GROUP: {
            char *group =
                rigth_pad(arena, file->group, fl->lens[LIST_ENUM_GROUP]);
            if (!group) {
                return false;
            }

            fl->wb_len +=
                ft_strlcpy(*output_str + fl->wb_len, group, fl->buffer_len - fl->wb_len);
            fl->wb_len +=
                ft_strlcpy(*output_str + fl->wb_len, " ", fl->buffer_len - fl->wb_len);
            break;
        }
        case LIST_ENUM_SIZE: {
            char *size =
                left_pad(arena, file->size->size, fl->lens[LIST_ENUM_SIZE]);
            if (!size) {
                return false;
            }

            fl->wb_len +=
                ft_strlcpy(*output_str + fl->wb_len, size, fl->buffer_len - fl->wb_len);
            fl->wb_len +=
                ft_strlcpy(*output_str + fl->wb_len, " ", fl->buffer_len - fl->wb_len);
            break;
        }
        case LIST_ENUM_DT:
            fl->wb_len += ft_strlcpy(*output_str + fl->wb_len, file->dt->str,
                                     fl->wb_len);
            fl->wb_len +=
                ft_strlcpy(*output_str + fl->wb_len, " ", fl->buffer_len - fl->wb_len);
            break;
        case LIST_ENUM_NAME:
            if (file->name->str[0] == '\'' || file->name->str[0] == '"') {
                fl->wb_len +=
                    ft_strlcpy(*output_str + fl->wb_len, " ", fl->buffer_len - fl->wb_len);
            }
            fl->wb_len += ft_strlcpy(*output_str + fl->wb_len,
                                     file->name->str, fl->buffer_len - fl->wb_len);
            break;
        case LIST_ENUM_LINK:

            if (file->linked_name) {
                fl->wb_len +=
                    ft_strlcpy(*output_str + fl->wb_len, " -> ", fl->buffer_len - fl->wb_len);
                fl->wb_len += ft_strlcpy(*output_str + fl->wb_len,
                                         file->linked_name->str, fl->buffer_len - fl->wb_len);
            }
            break;
        case LIST_ENUM_COUNT:
            break;
        default:
            ASSERT_(true == false, "Should never ever happen");
    }

    return true;
}

static bool calc_cols_(Arena *arena, t_path *path, size_t **col_widths,
                       t_map *map) {
    ASSERT_(arena, "arena con not be NULL");
    ASSERT_(path, "path can not be NULL");
    ASSERT_(path->files, "path->files can not be NULL");
    ASSERT_(path->files->len, "path->files->len must be > 0");
    ASSERT_(path->files->data, "path->files->data can not be NULL");
    ASSERT_(path->files->data[0], "path->files->data[0] can not be NULL");
    ASSERT_(col_widths, "col_widths can not be NULL");
    // ASSERT_(num_cols, "num_cols can not be NULL");
    // ASSERT_(*num_cols, "*num_cols must be > 0");
    // ASSERT_(num_rows, "num_rows can not be NULL");
    // ASSERT_(*num_rows, "*num_rows must be > 0");

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
            map->col = try_cols;
            map->row = (files_len + map->col - 1) / map->col;
            break;
        }
    }

    (void)calc_layout_width_(path->files, map->col, *col_widths);

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
#endif // __linux__
