#include <dirent.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../include/ft_arena.h"
#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_path.h"
#include "../include/ft_printer.h"
#include "../include/ft_sort.h"
#include "../include/ft_str.h"
#include "../include/ft_walk.h"

#include "../libft/include/ft_fprintf.h"

typedef struct {
    t_args *args;
    Arena *dirs_arena;
    Arena *files_arena;
    t_array *dirs;
    t_array *files;
} t_params;

static bool read_dir_(t_params *params, t_str *path, int *exit_code);
static bool process_args_(t_params *params, t_array *array, int *exit_code);
static t_str *join_paths_(Arena *files_arena, t_str *lhs, t_str *rhs);
static t_str *need_quote_(Arena *arena, t_str *str);
static void clean_up_(t_params *params);

typedef enum { SINGLE_QUOTE = '\'', DOUBLE_QUOTE = '\"', SPACE = ' ' } t_quote;

void process(t_args *args, t_array *array, int *exit_code) {
    ASSERT_NOTNULL(args);
    ASSERT_NOTNULL(array);
    ASSERT_GE(array->cap, 1);
    ASSERT_GE(array->len, 1);
    ASSERT_NOTNULL(exit_code);

    const char *err_msg = NULL;
    t_params params = {.args = args,
                       .dirs_arena = ArenaAlloc(ARENA_SIZE),
                       .files_arena = ArenaAlloc(ARENA_SIZE)};
    if (!params.dirs_arena) {
        err_msg = "Failed to alloc arena for dirs";
        goto failed;
    }

    if (!params.files_arena) {
        err_msg = "Failed to alloc arena for files";
        goto failed;
    }
    ArenaSetAutoAlign(params.dirs_arena, 8);
    ArenaSetAutoAlign(params.files_arena, 8);

    params.dirs = init_array(params.dirs_arena, ARRAY_SIZE);
    if (!params.dirs) {
        err_msg = "Failed to alloc files";
        goto failed;
    }

    params.files = init_array(params.files_arena, ARRAY_SIZE);
    if (!params.files) {
        err_msg = "Failed to alloc dirs";
        goto failed;
    }

    if (!process_args_(&params, array, exit_code)) {
        goto failed;
    }

    if (params.files->len) {
        printer(args, params.files);
        clear_array(params.files);
    }
    ArenaClear(params.files_arena);

    while (params.dirs->len > 0) {
        t_str *dir_path = pop_array(params.dirs);

        if (!read_dir_(&params, dir_path, exit_code)) {
            *exit_code = 2;
        }

        if (params.files->len) {
            printer(args, params.files);
            clear_array(params.files);
        }
        ArenaClear(params.files_arena);
    }

    clean_up_(&params);
    return;
failed:
    if (err_msg) {
        ft_fprintf(STDERR_FILENO, "Error: %s\n", err_msg);
    }

    clean_up_(&params);
}

static bool process_args_(t_params *params, t_array *array, int *exit_code) {
    ASSERT_NOTNULL(params);
    ASSERT_NOTNULL(params->args);
    ASSERT_NOTNULL(params->dirs_arena);
    ASSERT_NOTNULL(params->files_arena);
    ASSERT_NOTNULL(params->dirs);
    ASSERT_NOTNULL(params->files);
    ASSERT_NOTNULL(array);
    ASSERT_NOTNULL(exit_code);

    const char *err_msg = NULL;
    struct stat st;

    for (uint64_t index = 0; index < array->len; ++index) {
        t_str *str = array->data[index];

        if (lstat(str->str, &st) == -1) {
            ft_fprintf(STDERR_FILENO,
                       "ft_ls: cannot access '%s': No such file or directory\n",
                       str->str);
            *exit_code = 2;
            return NULL;
        }

        if (S_ISDIR(st.st_mode)) {
            if (!append_array(params->dirs, str)) {
                err_msg = "Failed to append dir";
                goto failed;
            }
            continue;
        }

        t_entry *entry = ArenaPush(params->files_arena, sizeof(*entry));
        if (!entry) {
            err_msg = "Failed to alloc entry";
            goto failed;
        }

        entry->quoted = need_quote_(params->files_arena, str);
        if (!entry->quoted) {
            err_msg = "Failed to create a quoted str";
            goto failed;
        }

        entry->name = str;
        entry->path = str;
        entry->st = st;

        if (!append_array(params->files, entry)) {
            err_msg = "Failed to append file entry";
            goto failed;
        }
    }

    return true;
failed:
    ft_fprintf(STDERR_FILENO, "Error: %s\n", err_msg);
    return false;
}

static bool read_dir_(t_params *params, t_str *path, int *exit_code) {
    ASSERT_NOTNULL(params);
    ASSERT_NOTNULL(params->args);
    ASSERT_NOTNULL(params->dirs_arena);
    ASSERT_NOTNULL(params->files_arena);
    ASSERT_NOTNULL(params->dirs);
    ASSERT_NOTNULL(params->files);
    ASSERT_NOTNULL(path);
    ASSERT_NOTNULL(exit_code);

    DIR *d = opendir(path->str);
    if (!d) {
        ft_fprintf(STDERR_FILENO,
                   "ft_ls: cannot open directory '%s': Permission denied\n",
                   path);
        return false;
    }

    t_array *entries = init_array(params->files_arena, ARRAY_SIZE);
    if (!entries) {
        goto failed;
    }

    struct dirent *dp;
    while ((dp = readdir(d)) != NULL) {
        if (!params->args->all && dp->d_name[0] == '.' &&
            dp->d_name[1] != '/') {
            continue;
        }

        t_entry *entry = ArenaPush(params->files_arena, sizeof(*entry));
        if (!entry) {
            goto failed;
        }

        entry->name = create_str(params->files_arena, dp->d_name);
        if (!entry->name) {
            goto failed;
        }

        entry->path = join_paths_(params->files_arena, path, entry->name);
        if (!entry->path) {
            goto failed;
        }

        entry->quoted = need_quote_(params->files_arena, entry->name);
        if (!entry->quoted) {
            goto failed;
        }

        if (lstat(entry->path->str, &entry->st) == -1) {
            *exit_code = 2;
            continue;
        }

        if (S_ISDIR(entry->st.st_mode) && params->args->recursive) {
            if (!append_array(entries, entry)) {
                goto failed;
            }
        } else {
            if (!append_array(params->files, entry)) {
                goto failed;
            }
        }
    }
    closedir(d);
    d = NULL;

    if (params->files->len) {
        printer(params->args, params->files);
        clear_array(params->files);
    }

    if (params->args->recursive) {
        sort(entries, params->args->recursive, params->args->time);
        size_t index = entries->len;
        while (index > 0) {
            --index;
            t_entry *entry = pop_array(entries);

            if (entry->name->str[0] == '.' &&
                (entry->name->str[1] == '\0' ||
                 (entry->name->str[1] == '.' && entry->name->str[2] == '\0'))) {
                continue;
            }

            t_str *dir_path = dup_str(params->dirs_arena, entry->path);
            if (!dir_path) {
                goto failed;
            }

            if (!append_array(params->dirs, dir_path)) {
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

static t_str *join_paths_(Arena *arena, t_str *lhs, t_str *rhs) {
    const size_t new_len = lhs->len + 1 + rhs->len + 1;
    t_str *fullname = init_str(arena, new_len);
    if (!fullname) {
        return NULL;
    }

    const uint64_t arena_pos = ArenaPos(arena);
    t_str *slash = create_str(arena, "/");
    if (!slash) {
        goto failed;
    }

    uint64_t len = cat_str(fullname, lhs);
    if (fullname->str[fullname->len - 1] != '/') {
        len += cat_str(fullname, slash);
    }
    len += cat_str(fullname, rhs);
    ASSERT_EQ(fullname->len, len);

    ArenaPopTo(arena, arena_pos);
    return fullname;

failed:
    ArenaPopTo(arena, arena_pos);
    return NULL;
}

static t_str *need_quote_(Arena *arena, t_str *str) {
    ASSERT_NOTNULL(arena);
    ASSERT_NOTNULL(str);
    ASSERT_GE(str->len, 1);
    ASSERT_LT(str->pos, str->len);

    t_str *quote = create_str(arena, " ");
    if (!quote) {
        return NULL;
    }

    while (has_next_str(str)) {
        const char lttr = next_str(str);
        switch (lttr) {
            case SINGLE_QUOTE: quote->str[0] = '"'; break;
            case DOUBLE_QUOTE: /* FALLTHROUGH */
            case SPACE: quote->str[0] = '\''; break;
            default: break;
        }
    }

    if (quote) {
        quote->len = 0;
    }

    return quote;
}

static void clean_up_(t_params *params) {
    if (params->dirs_arena) {
        ArenaRelease(params->dirs_arena);
    }

    if (params->files_arena) {
        ArenaRelease(params->files_arena);
    }
}
