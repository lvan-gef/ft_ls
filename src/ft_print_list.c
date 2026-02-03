#include <sys/stat.h>

#include "../include/ft_assert.h"
#include "../include/ft_helpers.h"
#include "../include/ft_ls.h"
#include "../include/ft_print_list.h"
#include "../include/ft_arena.h"
#include "../include/ft_get_stats.h"

#include "../libft/include/libft.h"

static size_t list_str_len_(Arena *scratch_arena, t_path *path, size_t **lens,
                            size_t *total);
// static char *rigth_pad_(Arena *arena, size_t nbr, size_t nbr_len);

bool print_list(t_path *path, t_array *files) {
    // ASSERT_(arena, "arena can not be NULL");
    ASSERT_(path, "path can not be NULL");
    ASSERT_(path->max_len, "path->max_len must be > 0");
    ASSERT_(path->name, "path->name can not be NULL");
    ASSERT_(path->name->str, "path->name->str can not be NULL");
    ASSERT_(*path->name->str, "*path->name->str can not be '\\0'");

    ASSERT_(files, "files can not be NULL");
    ASSERT_(files->len, "files->len must be > 0");
    ASSERT_(files->data, "files->data can not be NULL");
    ASSERT_(*files->data, "*path->files->data can not be NULL");

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
    output_arena = ArenaAlloc(str_len + header_len);
    if (!output_arena) {
        goto failed;
    }
    ArenaSetAutoAlign(output_arena, 8);

    char *output_str = ArenaPush(output_arena, str_len);
    if (!output_str) {
        goto failed;
    }

    size_t len = ft_strlcpy(output_str, "totaal ", str_len);
    len += uitoa(output_str + len, str_len, total / 2);
    len += ft_strlcpy(output_str + len, "\n", str_len);

    size_t index = 0;
    while (index < path->files->len) {
        t_file *file = path->files->data[index];
        size_t list_index = 0;
        while (list_index < LIST_ENUM_COUNT) {
            switch (file->list_types) {
                case LIST_ENUM_PERMISSION:
                    break;
                case LIST_ENUM_HARDLINK:
                    break;
                case LIST_ENUM_USER:
                    break;
                case LIST_ENUM_GROUP:
                    break;
                case LIST_ENUM_SIZE:
                    break;
                case LIST_ENUM_DT:
                    break;
                case LIST_ENUM_NAME:
                    break;
                case LIST_ENUM_LINK:
                    break;
                case LIST_ENUM_COUNT:
                    break;
                default:
                    ASSERT_(true == false, "Should never ever happen");
            }
            ++list_index;
        }
        // len = ft_strlcpy(total_str, file->permission, str_len);
        // len += ft_strlcpy(total_str + len, " ", str_len);
        //
        // char *hardlink = rigth_pad_(arena, file->hardlink, lens[1]);
        // if (!hardlink) {
        //     goto failed;
        // }
        // len += ft_strlcpy(total_str + len, hardlink, str_len);
        // len += ft_strlcpy(total_str + len, " ", str_len);
        //
        // size_t group_len = ft_strlcpy(total_str + len, file->group, str_len);
        // len += group_len;
        // while (group_len < lens[2]) {
        //     len += ft_strlcpy(total_str + len, " ", str_len);
        //     ++group_len;
        // }
        // len += ft_strlcpy(total_str + len, " ", str_len);
        //
        // size_t user_len = ft_strlcpy(total_str + len, file->user, str_len);
        // len += user_len;
        // while (user_len < lens[3]) {
        //     len += ft_strlcpy(total_str + len, " ", str_len);
        //     ++user_len;
        // }
        // len += ft_strlcpy(total_str + len, " ", str_len);
        //
        // char *size = rigth_pad_(arena, (size_t)file->size, lens[4]);
        // if (!size) {
        //     goto failed;
        // }
        // len += ft_strlcpy(total_str + len, size, str_len);
        // len += ft_strlcpy(total_str + len, " ", str_len);
        //
        // len += ft_strlcpy(total_str + len, file->date_fmt, str_len);
        // len += ft_strlcpy(total_str + len, " ", str_len);
        //
        // if (file->name->str[0] == '\'' || file->name->str[0] == '"') {
        //     len += ft_strlcpy(total_str + len, " ", str_len);
        // }
        // len += ft_strlcpy(total_str + len, file->name->str, str_len);
        // if (*file->linkedname) {
        //     len += ft_strlcpy(total_str + len, " -> ", str_len);
        //     len += ft_strlcpy(total_str + len, file->linkedname, str_len);
        // }
        //
        // len += ft_strlcpy(total_str + len, "\n", str_len);
        // if (write(STDOUT_FILENO, total_str, len) < 0) {
        //     goto failed;
        // }

        ++index;
        // ArenaClear(scratch_arena);
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
    size_t index = 0;
    struct stat sb;

    while (index < path->files->len) {
        t_file *file = path->files->data[index];
        t_str *fullname = join_paths(scratch_arena, path->name, file->name);
        if (!fullname) {
            return 0;
        }

        if (!get_stat(&sb, fullname->str)) {
            return 0;
        }

        size_t list_index = 0;
        while (list_index < LIST_ENUM_COUNT) {
            switch (file->list_types) {
                case LIST_ENUM_PERMISSION:
                    if (!get_permission(scratch_arena, &sb, file)) {
                        return 0;
                    }

                    if (file->permission->len > (*lens)[list_index]) {
                        (*lens)[list_index] = file->permission->len;
                    }
                    break;
                case LIST_ENUM_HARDLINK:
                    if (!get_hardlink(scratch_arena, &sb, file)) {
                        return 0;
                    }

                    if (file->hardlink->str->len > (*lens)[list_index]) {
                        (*lens)[list_index] = file->hardlink->count;
                    }
                    break;
                case LIST_ENUM_USER:
                    if (!get_user(scratch_arena, file, sb.st_uid)) {
                        return 0;
                    }

                    if (file->user->len > (*lens)[list_index]) {
                        (*lens)[list_index] = file->user->len;
                    }
                    break;
                case LIST_ENUM_GROUP:
                    if (!get_group(scratch_arena, file, sb.st_gid)) {
                        return 0;
                    }

                    if (file->group->len > (*lens)[list_index]) {
                        (*lens)[list_index] = file->group->len;
                    }
                    break;
                case LIST_ENUM_SIZE:
                    if (!get_size(scratch_arena, &sb, file)) {
                        return 0;
                    }

                    if (file->size->str->len > (*lens)[list_index]) {
                        (*lens)[list_index] = file->size->str->len;
                    }

                    ASSERT_(*total <= SIZE_MAX - file->blocks,
                            "total did overflow");
                    *total += file->blocks;
                    break;
                case LIST_ENUM_DT:
                    if (!get_dt(scratch_arena, &sb, file)) {
                        return 0;
                    }

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
                        size_t extra_space = 4; // ' -> '
                        size_t linked_len =
                            file->linked_name->len + extra_space;
                        if (linked_len > (*lens)[list_index]) {
                            (*lens)[list_index] = linked_len;
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

    while (index < 8) {
        str_len += (*lens)[index] + 2;
        ++index;
    }

    return str_len;
}

// static char *rigth_pad_(Arena *arena, size_t nbr, size_t nbr_len) {
//     ASSERT_(arena, "arena can not be NULL");
//     ASSERT_(nbr, "nbr must be > 0");
//     ASSERT_(nbr_len, "nbr_len must be > 0");
//
//     const size_t new_cap = nbr_len + 1;
//     ASSERT_(new_cap > nbr_len, "new_cap did overflow");
//
//     char *nbr_str = ArenaPush(arena, new_cap);
//     if (!nbr_str) {
//         return NULL;
//     }
//
//     size_t str_len = uitoa(nbr_str, new_cap, nbr);
//     U64 arena_pos = ArenaPos(arena);
//     char *buffer = ArenaPush(arena, new_cap);
//     if (!buffer) {
//         ArenaPopTo(arena, arena_pos);
//         return NULL;
//     }
//
//     size_t pad_count = (nbr_len > str_len) ? nbr_len - str_len : 0;
//     size_t len = 0;
//     while (len < pad_count) {
//         buffer[len] = ' ';
//         ++len;
//     }
//
//     len += ft_strlcpy(buffer + len, nbr_str, new_cap - len);
//     ASSERT_(len == nbr_len, "len != nbr_len");
//
//     ASSERT_(buffer, "buffer can not be NULL");
//     ASSERT_(*buffer, "*buffer can not be '\\0'");
//     return buffer;
// }
