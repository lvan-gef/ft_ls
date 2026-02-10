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

#include "../libft/include/ft_fprintf.h"

static bool read_dir_(t_args *args, Arena *files_arena, Arena *dirs_arena,
                      t_str *path, t_array *dirs, int *exit_code);
static bool process_args(Arena *files_arena, t_array *array, t_array *dirs,
                         t_array *files, int *exit_code);

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

    if (!process_args(files_arena, array, dirs, files, exit_code)) {
        goto failed;
    }

    // Phase 2: sort & print file arguments
    // TODO: sort file_entries
    size_t index = 0;
    while (index < files->len) {
        t_entry *entry = files->data[index];
        ft_fprintf(STDOUT_FILENO, "%s\n", entry->name->str);
        ++index;
    }

    // Phase 3: process directories (works for both -R and non-R)
    bool multiple = (files->len > 0) || (dirs->len > 0);
    while (dirs->len > 0) {
        t_str *dir_path = pop_array(dirs);

        if (multiple) {
            ft_fprintf(STDOUT_FILENO, "\n%s:\n", dir_path->str);
        }

        if (!read_dir_(args, files_arena, dirs_arena, dir_path, dirs,
                       exit_code)) {
            *exit_code = 2;
        }

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

static bool process_args(Arena *files_arena, t_array *array, t_array *dirs,
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
        struct stat st;

        if (lstat(str->str, &st) == -1) {
            ft_fprintf(STDERR_FILENO,
                       "ft_ls: cannot access '%s': No such file or directory\n",
                       str->str);
            *exit_code = 2;
            ++index;
            continue;
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

    t_str *slash = create_str(files_arena, "/");
    if (!slash) {
        goto failed;
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

        const size_t new_len = path->len + 1 + entry->name->len + 1;
        t_str *fullname = init_str(files_arena, new_len);
        if (!fullname) {
            goto failed;
        }

        uint64_t len = cat_str(fullname, path);
        if (fullname->str[fullname->len] != '\0') {
            len += cat_str(fullname, slash);
        }
        len += cat_str(fullname, entry->name);
        ASSERT_EQ(fullname->len, len);

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

    // TODO: sort entries

    // Print entries
    // uint64_t i = 0;
    // while (i < entries->len) {
    //     t_entry *entry = entries->data[i];
    //     ft_fprintf(STDOUT_FILENO, "--> %s\n", entry->name->str);
    //     ++i;
    // }

    // If -R, push subdirectories onto stack (reversed for correct order)
    if (args->recursive) {
        uint64_t index = entries->len;
        while (index > 0) {
            --index;
            t_entry *entry = entries->data[index];

            // Use d_type if available, fall back to lstat
            bool is_dir = false;
            if (!args->list && !args->time) {
                struct stat st;
                if (lstat(entry->path->str, &st) == 0) {
                    is_dir = S_ISDIR(st.st_mode);
                }
            } else {
                is_dir = S_ISDIR(entry->st.st_mode);
            }

            if (!is_dir) {
                continue;
            }

            // Skip "." and ".."
            if (entry->name->str[0] == '.' &&
                (entry->name->str[1] == '\0' ||
                 (entry->name->str[1] == '.' && entry->name->str[2] == '\0'))) {
                continue;
            }

            // Copy path to dirs arena so it survives ArenaClear(files)
            t_str *dir_path = dup_str(dirs_arena, entry->path);
            if (!dir_path || !append_array(dirs, dir_path)) {
                return false;
            }
        }
    }

    return true;

failed:
    closedir(d);
    return false;
}
