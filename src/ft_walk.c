#include <dirent.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../include/ft_arena.h"
#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_entry.h"
#include "../include/ft_helper.h"
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

static bool run_(t_args *args, t_params *params, t_array *array,
                 int *exit_code);
static bool read_dir_(t_params *params, t_entry *path, int *exit_code);
static bool walk_recurssive_(t_params *params);
static bool process_args_(t_params *params, t_array *array, int *exit_code);
static t_entry *create_entry_(Arena *arena, t_entry *path, struct dirent *dp);
static int check_links(t_params *params, t_str *str, t_array *dir_entries,
                       struct stat *st, int *exit_code);
static t_str *join_paths_(Arena *arena, const t_str *lhs, const t_str *rhs);
static t_str *need_quote_(Arena *arena, t_str *str);
static bool has_quote_char_(const t_str *str);
static bool has_quoted_operands_(const t_array *array);
static void clean_up_(t_params *params);
static void print_err_(Arena *arena, t_str *str, int e, const char *prefix);

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
        err_msg = "Failed to alloc entries";
        goto failed;
    }

    if (!run_(args, &params, array, exit_code)) {
        goto failed;
    }

    clean_up_(&params);
    return;
failed:
    if (err_msg) {
        ft_fprintf(STDERR_FILENO, "Error: %s\n", err_msg);
    }

    clean_up_(&params);
}

static bool run_(t_args *args, t_params *params, t_array *array,
                 int *exit_code) {
    ASSERT_NOTNULL(args);
    ASSERT_NOTNULL(params);
    ASSERT_NOTNULL(array);
    ASSERT_NOTNULL(exit_code);
    ASSERT_EQ(*exit_code, 0);

    const char *err_msg = NULL;
    if (!process_args_(params, array, exit_code)) {
        goto failed;
    }

    const bool has_quoted_operands = has_quoted_operands_(array);
    bool printed_files = false;
    bool printed_dir = false;
    bool inserted_files_dirs_gap = false;
    const bool print_dir_path = args->recursive || array->len > 1;

    if (params->files->len) {
        printer(args, params->files, NULL, false, params->max_len_links,
                params->max_len_sizes, has_quoted_operands);
        printed_files = true;
        params->files = reset_array(params->files_arena);
        if (!params->files) {
            err_msg = "Failed to reset files array";
            goto failed;
        }
    }

    while (params->dirs->len) {
        t_entry *dir_path = pop_array(params->dirs);

        if (!inserted_files_dirs_gap && printed_files && dir_path->is_operand) {
            write(STDOUT_FILENO, "\n", 1);
            inserted_files_dirs_gap = true;
        }

        if (!read_dir_(params, dir_path, exit_code)) {
            params->files = reset_array(params->files_arena);
            if (!params->files) {
                err_msg = "Failed to reset files array";
                goto failed;
            }
            continue;
        }

        if (printed_dir) {
            write(STDOUT_FILENO, "\n", 1);
        }

        t_entry entry = {.name = dir_path->path};
        entry.quoted = need_quote_(params->entries_arena, entry.name);
        if (!entry.quoted) {
            err_msg = "Failed to get quote";
            goto failed;
        }

        t_entry *ent = print_dir_path ? &entry : NULL;
        printer(args, params->files, ent, true, 0, 0, false);
        printed_dir = true;
        params->files = reset_array(params->files_arena);
        if (!params->files) {
            err_msg = "Failed to reset files array";
            goto failed;
        }
    }

    return true;
failed:
    if (err_msg) {
        ft_fprintf(STDERR_FILENO, "Error: %s\n", err_msg);
    }

    return false;
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

        int state = check_links(params, str, dir_entries, &st, exit_code);
        if (state == 0) {
            continue;
        } else if (state < 0) {
            goto failed;
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
        sort(params->scratch_arena, dir_entries, params->args->reverse,
             params->args->time);
        while (dir_entries->len) {
            t_entry *entry = pop_array(dir_entries);
            entry->is_operand = true;
            if (!append_array(params->dirs, entry)) {
                err_msg = "Failed to append dir";
                goto failed;
            }
        }
    }

    return true;
failed:
    ft_fprintf(STDERR_FILENO, "Error: %s\n", err_msg);
    *exit_code = 2;
    return false;
}

static t_entry *create_entry_(Arena *arena, t_entry *path, struct dirent *dp) {
    ASSERT_NOTNULL(arena);
    ASSERT_NOTNULL(path);
    ASSERT_NOTNULL(dp);

    ArenaMark marker = ArenaGetMark(arena);
    t_entry *entry = ArenaPush(arena, sizeof(*entry));
    if (!entry) {
        goto failed;
    }

    entry->name = create_str(arena, dp->d_name);
    if (!entry->name) {
        goto failed;
    }

    entry->path = join_paths_(arena, path->path, entry->name);
    if (!entry->path) {
        goto failed;
    }

    entry->quoted = need_quote_(arena, entry->name);
    if (!entry->quoted) {
        goto failed;
    }

    return entry;
failed:
    ArenaPopToMark(arena, marker);
    return NULL;
}

static int check_links(t_params *params, t_str *str, t_array *dir_entries,
                       struct stat *st, int *exit_code) {
    const char *err_msg = NULL;
    if (lstat(str->str, st) == -1) {
        int e = errno;
        const char *prefix = "cannot access";
        print_err_(params->files_arena, str, e, prefix);
        *exit_code = 2;
        return 0;
    }

    if (params->args->list) {
        const uint64_t links_len = len_of_nbr((uint64_t)st->st_nlink);
        if (links_len > params->max_len_links) {
            params->max_len_links = links_len;
        }

        const uint64_t size_len = len_of_nbr((uint64_t)st->st_size);
        if (size_len > params->max_len_sizes) {
            params->max_len_sizes = size_len;
        }
    }

    bool is_dir_operand = S_ISDIR(st->st_mode);
    struct stat *st_dir = st;
    if (S_ISLNK(st->st_mode)) {
        struct stat *st_target = NULL;
        if (stat(str->str, st_target) == 0 && S_ISDIR(st_target->st_mode)) {
            is_dir_operand = true;
            st_dir = st_target;
        }
    }

    if (is_dir_operand && !params->args->list) {
        t_entry *dir_entry = ArenaPush(params->files_arena, sizeof(*dir_entry));
        if (!dir_entry) {
            err_msg = "Failed to alloc dir entry";
            goto failed;
        }

        dir_entry->name = str;
        dir_entry->path = str;
        dir_entry->st = *st_dir;
        if (!append_array(dir_entries, dir_entry)) {
            err_msg = "Failed to append dir entry";
            goto failed;
        }
        return 0;
    }

    return 1;
failed:
    if (err_msg) {
        ft_fprintf(STDERR_FILENO, "%s\n", err_msg);
    }

    return -1;
}

static bool read_dir_(t_params *params, t_entry *path, int *exit_code) {
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

    errno = 0;
    DIR *d = opendir(path->path->str);
    if (!d) {
        int e = errno;
        const char *prefix = "cannot open directory";
        print_err_(params->files_arena, path->path, e, prefix);
        *exit_code = (path->is_operand ? 2 : 1);
        return false;
    }

    params->entries = reset_array(params->entries_arena);
    if (!params->entries) {
        goto failed;
    }

    struct dirent *dp;
    while ((dp = readdir(d)) != NULL) {
        if (!params->args->all && dp->d_name[0] == '.' &&
            dp->d_name[1] != '/') {
            continue;
        }

        t_entry *entry = create_entry_(params->entries_arena, path, dp);
        if (!entry) {
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

    return walk_recurssive_(params);
failed:
    closedir(d);
    *exit_code = 2;
    return false;
}

static bool walk_recurssive_(t_params *params) {
    ASSERT_NOTNULL(params);

    if (params->args->recursive && params->entries->len) {
        sort(params->scratch_arena, params->entries, params->args->reverse,
             params->args->time);
        size_t index = params->entries->len;
        while (index > 0) {
            --index;
            t_entry *entry = pop_array(params->entries);
            if (!entry->name) {
                continue;
            }

            if (entry->name->str[0] == '.' &&
                (entry->name->str[1] == '\0' ||
                 (entry->name->str[1] == '.' && entry->name->str[2] == '\0'))) {
                continue;
            }

            entry->is_operand = false;
            if (!append_array(params->dirs, entry)) {
                return false;
            }
        }
    }

    return true;
}

static t_str *join_paths_(Arena *arena, const t_str *lhs, const t_str *rhs) {
    const size_t new_len = lhs->len + 1 + rhs->len + 1;
    t_str *fullname = init_str(arena, new_len);
    if (!fullname) {
        return NULL;
    }

    char slash_buffer[] = "/";
    t_str slash = {.str = slash_buffer, .cap = 2, .len = 1, .pos = 0};
    (void)cat_str(fullname, lhs);
    if (fullname->str[fullname->len - 1] != '/') {
        (void)cat_str(fullname, &slash);
    }
    (void)cat_str(fullname, rhs);

    return fullname;
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
    bool found_double = false;
    while (has_next_str(str)) {
        const char lttr = next_str(str);
        switch (lttr) {
            case SINGLE_QUOTE: quote->str[0] = '"'; break;
            case DOUBLE_QUOTE:
                found_double = true;
                quote->str[0] = '\'';
                break;
            case SPACE: quote->str[0] = '\''; break;
            default: break;
        }
    }

    if (quote->str[0] == '"' && found_double) {
        quote->str[0] = '\'';
    }

    str->pos = cur_pos;
    if (peek_str(quote) == ' ') {
        quote->len = 0;
    }

    return quote;
}

static bool has_quote_char_(const t_str *str) {
    ASSERT_NOTNULL(str);
    ASSERT_NOTNULL(str->str);

    for (uint64_t index = 0; index < str->len; ++index) {
        const char letter = str->str[index];
        switch (letter) {
            case SINGLE_QUOTE:
            case DOUBLE_QUOTE:
            case SPACE: return true;
            default: continue;
        }
    }

    return false;
}

static bool has_quoted_operands_(const t_array *array) {
    ASSERT_NOTNULL(array);

    for (uint64_t index = 0; index < array->len; ++index) {
        const t_str *operand = array->data[index];
        if (!operand) {
            continue;
        }

        if (has_quote_char_(operand)) {
            return true;
        }
    }

    return false;
}

static void clean_up_(t_params *params) {
    ASSERT_NOTNULL(params);

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

static void print_err_(Arena *arena, t_str *str, int e, const char *prefix) {
    const char *msg = strerror(e);
    t_str *quote = need_quote_(arena, str);

    if (quote && quote->len) {
        if (quote->str[0] == '"') {
            ft_fprintf(STDERR_FILENO, "ft_ls: %s %s%s%s: %s\n", prefix,
                       quote->str, str->str, quote->str, msg);
            return;
        }

        t_str *new_str = escape_str(arena, str);
        if (new_str) {
            ft_fprintf(STDERR_FILENO, "ft_ls: %s %s%s%s: %s\n", prefix,
                       quote->str, new_str->str, quote->str, msg);
            return;
        }
    }

    ft_fprintf(STDERR_FILENO, "ft_ls: %s '%s': %s\n", prefix, str->str, msg);
}
