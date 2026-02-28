#include <dirent.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../include/ft_arena.h"
#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_helper.h"
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
    Arena *entries_arena;
    Arena *scratch_arena;
    t_array *dirs;
    t_array *files;
    t_array *entries;
    uint64_t max_len_links;
    uint64_t max_len_sizes;
} t_params;

static bool read_dir_(t_params *params, t_str *path, int *exit_code);
static bool process_args_(t_params *params, t_array *array, int *exit_code);
static bool reset_files_(t_params *params);
static bool reset_entries_(t_params *params);
static t_str *join_paths_(Arena *arena, const t_str *lhs, const t_str *rhs);
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
                       .files_arena = ArenaAlloc(ARENA_SIZE),
                       .entries_arena = ArenaAlloc(ARENA_SIZE),
                       .scratch_arena = ArenaAlloc(ARENA_SIZE)};
    if (!params.dirs_arena) {
        err_msg = "Failed to alloc arena for dirs";
        goto failed;
    }

    if (!params.files_arena) {
        err_msg = "Failed to alloc arena for files";
        goto failed;
    }

    if (!params.entries_arena) {
        err_msg = "Failed to alloc arena for entries";
        goto failed;
    }

    if (!params.scratch_arena) {
        err_msg = "Failed to alloc arena for scratch";
        goto failed;
    }

    ArenaSetAutoAlign(params.dirs_arena, 8);
    ArenaSetAutoAlign(params.files_arena, 8);
    ArenaSetAutoAlign(params.entries_arena, 8);
    ArenaSetAutoAlign(params.scratch_arena, 8);

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

    params.entries = init_array(params.entries_arena, ARRAY_SIZE);
    if (!params.entries) {
        goto failed;
    }

    if (!process_args_(&params, array, exit_code)) {
        goto failed;
    }

    bool printed_section = false;
    const bool print_dir_path = args->recursive || array->len > 1;

    if (params.files->len) {
        printer(args, params.files, NULL, false, params.max_len_links,
                params.max_len_sizes);
        printed_section = true;
        if (!reset_files_(&params)) {
            err_msg = "Failed to reset files array";
            goto failed;
        }
    }

    while (params.dirs->len) {
        t_str *dir_path = pop_array(params.dirs);

        if (printed_section) {
            write(STDOUT_FILENO, "\n", 1);
        }

        if (!read_dir_(&params, dir_path, exit_code)) {
            *exit_code = 2;
        }

        printer(args, params.files, print_dir_path ? dir_path : NULL, true, 0,
                0);
        printed_section = true;
        if (!reset_files_(&params)) {
            err_msg = "Failed to reset files array";
            goto failed;
        }
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
    ASSERT_NOTNULL(params->scratch_arena);
    ASSERT_NOTNULL(params->dirs);
    ASSERT_NOTNULL(params->files);
    ASSERT_NOTNULL(array);
    ASSERT_NOTNULL(exit_code);

    const char *err_msg = NULL;
    struct stat st;
    t_array *dir_entries = init_array(params->files_arena, ARRAY_SIZE);
    if (!dir_entries) {
        err_msg = "Failed to init dir entries";
        goto failed;
    }

    for (uint64_t index = 0; index < array->len; ++index) {
        t_str *str = array->data[index];

        if (lstat(str->str, &st) == -1) {
            ft_fprintf(STDERR_FILENO,
                       "ft_ls: cannot access '%s': No such file or directory\n",
                       str->str);
            *exit_code = 2;
            continue;
        }

        if (params->args->list) {
            const uint64_t links_len = len_of_nbr((uint64_t)st.st_nlink);
            if (links_len > params->max_len_links) {
                params->max_len_links = links_len;
            }

            const uint64_t size_len = len_of_nbr((uint64_t)st.st_size);
            if (size_len > params->max_len_sizes) {
                params->max_len_sizes = size_len;
            }
        }

        bool is_dir_operand = S_ISDIR(st.st_mode);
        struct stat st_dir = st;
        if (S_ISLNK(st.st_mode)) {
            struct stat st_target;
            if (stat(str->str, &st_target) == 0 && S_ISDIR(st_target.st_mode)) {
                is_dir_operand = true;
                st_dir = st_target;
            }
        }

        if (is_dir_operand && !params->args->list) {
            t_entry *dir_entry =
                ArenaPush(params->files_arena, sizeof(*dir_entry));
            if (!dir_entry) {
                err_msg = "Failed to alloc dir entry";
                goto failed;
            }

            dir_entry->name = str;
            dir_entry->path = str;
            dir_entry->st = st_dir;
            if (!append_array(dir_entries, dir_entry)) {
                err_msg = "Failed to append dir entry";
                goto failed;
            }
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            t_entry *dir_entry =
                ArenaPush(params->files_arena, sizeof(*dir_entry));
            if (!dir_entry) {
                err_msg = "Failed to alloc dir entry";
                goto failed;
            }

            dir_entry->name = str;
            dir_entry->path = str;
            dir_entry->st = st;
            if (!append_array(dir_entries, dir_entry)) {
                err_msg = "Failed to append dir entry";
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
        if (params->args->list) {
            if (!get_file_info(params->files_arena, params->scratch_arena,
                               entry)) {
                goto failed;
            }
        }

        if (!append_array(params->files, entry)) {
            err_msg = "Failed to append file entry";
            goto failed;
        }
    }

    if (dir_entries->len) {
        sort(dir_entries, params->args->reverse, params->args->time);
        while (dir_entries->len) {
            t_entry *entry = pop_array(dir_entries);
            if (!append_array(params->dirs, entry->path)) {
                err_msg = "Failed to append dir";
                goto failed;
            }
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
    ASSERT_NOTNULL(params->entries_arena);
    ASSERT_NOTNULL(params->scratch_arena);
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

    if (!reset_entries_(params)) {
        goto failed;
    }

    const struct dirent *dp;
    while ((dp = readdir(d)) != NULL) {
        if (!params->args->all && dp->d_name[0] == '.' &&
            dp->d_name[1] != '/') {
            continue;
        }

        t_entry *entry = ArenaPush(params->entries_arena, sizeof(*entry));
        if (!entry) {
            goto failed;
        }

        entry->name = create_str(params->entries_arena, dp->d_name);
        if (!entry->name) {
            goto failed;
        }

        entry->path = join_paths_(params->entries_arena, path, entry->name);
        if (!entry->path) {
            goto failed;
        }

        entry->quoted = need_quote_(params->entries_arena, entry->name);
        if (!entry->quoted) {
            goto failed;
        }

        if (lstat(entry->path->str, &entry->st) == -1) {
            *exit_code = 2;
            continue;
        }

        if (S_ISDIR(entry->st.st_mode) && params->args->recursive) {
            if (!append_array(params->entries, entry)) {
                goto failed;
            }
        }

        if (params->args->list) {
            if (!get_file_info(params->files_arena, params->scratch_arena,
                               entry)) {
                goto failed;
            }
        }
        if (!append_array(params->files, entry)) {
            goto failed;
        }
    }
    closedir(d);
    d = NULL;

    if (params->args->recursive && params->entries->len) {
        sort(params->entries, params->args->reverse, params->args->time);
        size_t index = params->entries->len;
        while (index > 0) {
            --index;
            const t_entry *entry = pop_array(params->entries);
            if (!entry->name) {
                continue;
            }

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
                goto failed;
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

static bool reset_files_(t_params *params) {
    ASSERT_NOTNULL(params);
    ASSERT_NOTNULL(params->files_arena);

    ArenaClear(params->files_arena);
    params->files = init_array(params->files_arena, ARRAY_SIZE);
    if (!params->files) {
        return false;
    }

    return true;
}

static bool reset_entries_(t_params *params) {
    ASSERT_NOTNULL(params);
    ASSERT_NOTNULL(params->entries_arena);

    ArenaClear(params->entries_arena);
    params->entries = init_array(params->entries_arena, ARRAY_SIZE);
    if (!params->entries) {
        return false;
    }

    return true;
}

static t_str *join_paths_(Arena *arena, const t_str *lhs, const t_str *rhs) {
    const ArenaMark mark = ArenaGetMark(arena);
    const size_t new_len = lhs->len + 1 + rhs->len + 1;
    t_str *fullname = init_str(arena, new_len);
    if (!fullname) {
        return NULL;
    }

    const ArenaMark scratch_mark = ArenaGetMark(arena);
    const t_str *slash = create_str(arena, "/");
    if (!slash) {
        goto failed;
    }

    (void)cat_str(fullname, lhs);
    if (fullname->str[fullname->len - 1] != '/') {
        (void)cat_str(fullname, slash);
    }
    (void)cat_str(fullname, rhs);

    ArenaPopToMark(arena, scratch_mark);
    return fullname;

failed:
    ArenaPopToMark(arena, mark);
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

    const uint64_t cur_pos = str->pos;
    while (has_next_str(str)) {
        const char lttr = next_str(str);
        switch (lttr) {
            case SINGLE_QUOTE: quote->str[0] = '"'; break;
            case DOUBLE_QUOTE: /* FALLTHROUGH */
            case SPACE: quote->str[0] = '\''; break;
            default: break;
        }
    }

    str->pos = cur_pos;
    if (peek_str(quote) == ' ') {
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

    if (params->entries_arena) {
        ArenaRelease(params->entries_arena);
    }

    if (params->scratch_arena) {
        ArenaRelease(params->scratch_arena);
    }
}
