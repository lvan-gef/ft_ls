#include <dirent.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../include/ft_arena.h"
#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_path.h"
#include "../include/ft_str.h"
#include "../include/ft_walk.h"
#include "../include/ft_sort.h"

#include "../libft/include/ft_fprintf.h"

static bool read_dir_(t_args *args, Arena *files_arena, Arena *dirs_arena,
                      t_str *path, t_array *dirs, int *exit_code);
static bool process_args_(Arena *files_arena, t_array *array, t_array *dirs,
                          t_array *files, int *exit_code);
static const char *check_input_(Arena *files_arena, t_array *dirs,
                                t_array *files, t_str *str, int *exit_code);
static t_str *join_paths(Arena *files_arena, t_str *lhs, t_str *rhs);

void process(t_args *args, t_array *array, int *exit_code) {
    ASSERT_NOTNULL(args);
    ASSERT_NOTNULL(array);
    ASSERT_GE(array->cap, 1);
    ASSERT_GE(array->len, 1);
    ASSERT_NOTNULL(exit_code);

    const char *err_msg = NULL;
    Arena *dirs_arena = NULL;
    Arena *files_arena = NULL;

    dirs_arena = ArenaAlloc(ARENA_SIZE);
    if (!dirs_arena) {
        err_msg = "Failed to alloc arena for dirs";
        goto failed;
    }
    ArenaSetAutoAlign(dirs_arena, 8);

    files_arena = ArenaAlloc(ARENA_SIZE);
    if (!files_arena) {
        err_msg = "Failed to alloc arena for files";
        goto failed;
    }
    ArenaSetAutoAlign(files_arena, 8);

    t_array *files = init_array(files_arena, ARRAY_SIZE);
    if (!files) {
        err_msg = "Failed to alloc files";
        goto failed;
    }

    t_array *dirs = init_array(dirs_arena, ARRAY_SIZE);
    if (!dirs) {
        err_msg = "Failed to alloc dirs";
        goto failed;
    }

    if (!process_args_(files_arena, array, dirs, files, exit_code)) {
        goto failed;
    }

    // sort & print file arguments
    sort(array, args->recursive, args->time);
    size_t index = 0;
    ft_fprintf(STDOUT_FILENO, "voor loop: %d\n", files->len);
    while (index < files->len) {
        t_entry *entry = files->data[index];
        ft_fprintf(STDOUT_FILENO, "%s\n", entry->name->str);
        ++index;
    }

    // Phase 3: process directories (works for both -R and non-R)
    // bool multiple = (files->len > 0) || (dirs->len > 0);
    while (dirs->len > 0) {
        t_str *dir_path = pop_array(dirs);

        // if (multiple) {
        //     ft_fprintf(STDOUT_FILENO, "\n%s:\n", dir_path->str);
        // }

        if (!read_dir_(args, files_arena, dirs_arena, dir_path, dirs,
                       exit_code)) {
            *exit_code = 2;
        }

        // print entrys
        ArenaClear(files_arena);
    }

    ArenaRelease(dirs_arena);
    ArenaRelease(files_arena);
    return;
failed:
    if (err_msg) {
        ft_fprintf(STDERR_FILENO, "%s\n", err_msg);
    }

    if (dirs_arena) {
        ArenaRelease(dirs_arena);
    }
    if (files_arena) {
        ArenaRelease(files_arena);
    }
}

static bool process_args_(Arena *files_arena, t_array *array, t_array *dirs,
                          t_array *files, int *exit_code) {
    ASSERT_NOTNULL(files_arena);
    ASSERT_NOTNULL(array);
    ASSERT_NOTNULL(dirs);
    ASSERT_NOTNULL(files);
    ASSERT_NOTNULL(exit_code);

    const char *err_msg = NULL;
    uint64_t index = 0;

    while (index < array->len) {
        t_str *str = array->data[index];

        err_msg = check_input_(files_arena, dirs, files, str, exit_code);
        if (err_msg) {
            goto failed;
        }

        ++index;
    }

    return true;
failed:
    ft_fprintf(STDERR_FILENO, "%s\n", err_msg);
    return false;
}

static bool read_dir_(t_args *args, Arena *files_arena, Arena *dirs_arena,
                      t_str *path, t_array *dirs, int *exit_code) {
    ASSERT_NOTNULL(args);
    ASSERT_NOTNULL(files_arena);
    ASSERT_NOTNULL(dirs_arena);
    ASSERT_NOTNULL(path);
    ASSERT_NOTNULL(dirs);
    ASSERT_NOTNULL(exit_code);

    DIR *d = opendir(path->str);
    if (!d) {
        ft_fprintf(STDERR_FILENO,
                   "ft_ls: cannot open directory '%s': Permission denied\n",
                   path);
        return false;
    }

    t_array *entries = init_array(files_arena, ARRAY_SIZE);
    if (!entries) {
        goto failed;
    }

    struct dirent *dp;
    while ((dp = readdir(d)) != NULL) {
        if (!args->all && dp->d_name[0] == '.') {
            continue;
        }

        t_entry *entry = ArenaPush(files_arena, sizeof(*entry));
        if (!entry) {
            goto failed;
        }

        entry->name = create_str(files_arena, dp->d_name);
        if (!entry->name) {
            goto failed;
        }

        t_str *fullname = join_paths(files_arena, path, entry->name);
        if (!fullname) {
            goto failed;
        }

        entry->path = fullname;
        if (args->list || args->time) {
            if (lstat(fullname->str, &entry->st) == -1) {
                *exit_code = 2;
                continue;
            }
        }

        if (!append_array(entries, entry)) {
            goto failed;
        }
    }
    closedir(d);
    d = NULL;

    sort(entries, args->recursive, args->time);
    // TODO: Print entries

    if (args->recursive) {
        uint64_t index = entries->len;
        while (index > 0) {
            --index;
            t_entry *entry = entries->data[index];

            bool is_dir = S_ISDIR(entry->st.st_mode);
            if (!is_dir) {
                continue;
            }

            // Skip "." and ".."
            if (entry->name->str[0] == '.' &&
                (entry->name->str[1] == '\0' ||
                 (entry->name->str[1] == '.' && entry->name->str[2] == '\0'))) {
                continue;
            }

            t_str *dir_path = dup_str(dirs_arena, entry->path);
            if (!dir_path) {
                goto failed;
            }

            if (!append_array(dirs, dir_path)) {
                return false;
            }
        }
    }

    return true;
failed:
    if (d) {
        closedir(d);
    }

    return false;
}

static const char *check_input_(Arena *files_arena, t_array *dirs,
                                t_array *files, t_str *str, int *exit_code) {
    const char *err_msg = NULL;
    struct stat st;

    if (lstat(str->str, &st) == -1) {
        ft_fprintf(STDERR_FILENO,
                   "ft_ls: cannot access '%s': No such file or directory\n",
                   str->str);
        *exit_code = 2;
        return NULL;
    }

    if (S_ISDIR(st.st_mode)) {
        if (!append_array(dirs, str)) {
            err_msg = "Failed to append dir";
            goto failed;
        }
    } else {
        t_entry *entry = ArenaPush(files_arena, sizeof(*entry));
        if (!entry) {
            err_msg = "Failed to alloc entry";
            goto failed;
        }

        entry->name = str;
        entry->path = str;
        entry->st = st;

        if (!append_array(files, entry)) {
            err_msg = "Failed to append file entry";
            goto failed;
        }
    }

    return NULL;
failed:
    return err_msg;
}

static t_str *join_paths(Arena *files_arena, t_str *lhs, t_str *rhs) {
    const uint64_t arena_pos = ArenaPos(files_arena);
    t_str *slash = create_str(files_arena, "/");
    if (!slash) {
        return NULL;
    }

    const size_t new_len = lhs->len + 1 + rhs->len + 1;
    t_str *fullname = init_str(files_arena, new_len);
    if (!fullname) {
        goto failed;
    }

    uint64_t len = cat_str(fullname, lhs);
    if (fullname->str[fullname->len] != '\0') {
        len += cat_str(fullname, slash);
    }
    len += cat_str(fullname, rhs);
    ASSERT_EQ(fullname->len, len);

    ArenaPopTo(files_arena, arena_pos);
    return fullname;

failed:
    ArenaPopTo(files_arena, arena_pos);
    return NULL;
}
