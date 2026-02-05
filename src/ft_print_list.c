#include <stdio.h>
#include <sys/stat.h>

#include "../include/ft_arena.h"
#include "../include/ft_assert.h"
#include "../include/ft_helpers.h"
#include "../include/ft_ls.h"
#include "../include/ft_print_list.h"

#include "../libft/include/libft.h"
#include "ft_printer_linux.h"

static size_t list_str_len_(Arena *scratch_arena, t_path *path, size_t **lens,
                            size_t *total);

bool print_list(t_path *path, t_array *files, bool print_header) {
    ASSERT_(path, "path can not be NULL");
    ASSERT_(path->name, "path->name can not be NULL");
    ASSERT_(path->name->str, "path->name->str can not be NULL");
    ASSERT_(*path->name->str, "*path->name->str can not be '\\0'");

    ASSERT_(files, "files can not be NULL");
    ASSERT_(files->data, "files->data can not be NULL");

    Arena *output_arena = NULL;
    Arena *scratch_arena = ArenaAlloc(ARENA_SIZE);
    if (!scratch_arena) {
        return false;
    }

    size_t *lens = ArenaPush(scratch_arena, LIST_ENUM_COUNT * sizeof(*lens));
    if (!lens) {
        goto failed;
    }
    ArenaSetAutoAlign(scratch_arena, 8);

    size_t total = 0;
    size_t char_count = list_str_len_(scratch_arena, path, &lens, &total);
    if (!char_count) {
        goto failed;
    }

    size_t str_len = ((char_count + 1) * files->len) * sizeof(char);
    size_t header_len = (7 + get_len(total) + 1) * sizeof(char);
    t_file_list fl = {.buffer_len = header_len + str_len + 1, .lens = lens};
    output_arena = ArenaAlloc(fl.buffer_len);
    if (!output_arena) {
        goto failed;
    }
    ArenaSetAutoAlign(output_arena, 8);

    char *output_str = ArenaPush(output_arena, fl.buffer_len);
    if (!output_str) {
        goto failed;
    }
    fl.buffer_len = str_len + header_len + 1;

    if (print_header) {
        if (write(STDOUT_FILENO, path->name->str, path->name->len) < 0) {
            return false;
        }

        if (write(STDOUT_FILENO, ":\n", 2) < 0) {
            return false;
        }
    }

    if (path->print_total) {
        fl.wb_len = ft_strlcpy(output_str, "total ", fl.buffer_len - fl.wb_len);
#if defined(__APPLE__)
        fl.wb_len += uitoa(output_str + fl.wb_len, fl.buffer_len, total);
#else
        fl.wb_len += uitoa(output_str + fl.wb_len, fl.buffer_len, total / 2);
#endif
        fl.wb_len +=
            ft_strlcpy(output_str + fl.wb_len, "\n", fl.buffer_len - fl.wb_len);
    }

    size_t index = 0;
    while (index < path->files->len) {
        t_file *file = path->files->data[index];
        fl.list_index = 0;

        while (fl.list_index < LIST_ENUM_COUNT) {
#if defined(__linux)
            if (!linux_list_format(scratch_arena, &fl, file, &output_str)) {
                goto failed;
            }
#endif
            ++fl.list_index;
        }

        ++index;
        fl.wb_len +=
            ft_strlcpy(output_str + fl.wb_len, "\n", fl.buffer_len - fl.wb_len);
    }

    if (write(STDOUT_FILENO, output_str, fl.wb_len) < 0) {
        return false;
    }

    ArenaRelease(scratch_arena);
    ArenaRelease(output_arena);
    return true;
failed:
    // TODO: print error
    ArenaRelease(scratch_arena);

    if (output_arena) {
        ArenaRelease(output_arena);
    }
    return false;
}

static size_t list_str_len_(Arena *scratch_arena, t_path *path, size_t **lens,
                            size_t *total) {
    ASSERT_(scratch_arena, "scratch_arena can not be NULL");
    ASSERT_(path, "path can not be NULL");
    ASSERT_(path->files, "path->files can not be NULL");
    ASSERT_(lens, "lens can not be NULL");
    ASSERT_(*lens, "*lens can not be NULL");
    ASSERT_(total, "total can not be NULL");
    ASSERT_(!*total, "*total must be 0");

    size_t index = 0;

    while (index < path->files->len) {
        t_file *file = path->files->data[index];
        ASSERT_(file, "file can not be NULL");
        ASSERT_(file->permission, "file->permission can not be NULL");
        ASSERT_(file->hardlink, "file->hardlink can not be NULL");
        ASSERT_(file->user, "file->user can not be NULL");
        ASSERT_(file->group, "file->group can not be NULL");
        ASSERT_(file->size, "file->size can not be NULL");
        ASSERT_(file->dt, "file->dt can not be NULL");
        ASSERT_(file->name, "file->name can not be NULL");

        t_str *fullname = join_paths(scratch_arena, path->name, file->name);
        if (!fullname) {
            return 0;
        }

        size_t list_index = 0;
        while (list_index < LIST_ENUM_COUNT) {
            switch (list_index) {
                case LIST_ENUM_PERMISSION:
                    if (file->permission->len > (*lens)[list_index]) {
                        (*lens)[list_index] = file->permission->len;
                    }
                    break;
                case LIST_ENUM_HARDLINK:
                    if (file->hardlink->str->len > (*lens)[list_index]) {
                        (*lens)[list_index] = file->hardlink->str->len;
                    }
                    break;
                case LIST_ENUM_USER:
                    if (file->user->len > (*lens)[list_index]) {
                        (*lens)[list_index] = file->user->len;
                    }
                    break;
                case LIST_ENUM_GROUP:
                    if (file->group->len > (*lens)[list_index]) {
                        (*lens)[list_index] = file->group->len;
                    }
                    break;
                case LIST_ENUM_SIZE:
                    if (file->size->str->len > (*lens)[list_index]) {
                        (*lens)[list_index] = file->size->str->len;
                    }

                    ASSERT_(*total <= SIZE_MAX - file->blocks,
                            "total did overflow");
                    *total += file->blocks;
                    break;
                case LIST_ENUM_DT:
                    if (file->dt->len > (*lens)[list_index]) {
                        (*lens)[list_index] = file->dt->len;
                    }

                    break;
                case LIST_ENUM_NAME:
                    if (file->name->len > (*lens)[list_index]) {
                        (*lens)[list_index] = file->name->len;
                    }
                    break;
                case LIST_ENUM_LINK:
                    if (file->linked_name) {
                        if (file->linked_name->len > (*lens)[list_index]) {
                            (*lens)[list_index] = file->linked_name->len;
                        }
                    }
                    break;
                case LIST_ENUM_COUNT:
                    break;
                default:
                    ASSERT_(true == false, "Should never ever happen");
            }
            ++list_index;
        }

        ++index;
    }

    size_t str_len = 0;
    index = 0;
    while (index < LIST_ENUM_COUNT) {
        str_len += (*lens)[index] + 2;
        ++index;
    }

    ASSERT_(str_len, "str_len must be > 0");
    return str_len;
}
