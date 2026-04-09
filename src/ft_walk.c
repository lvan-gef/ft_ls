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
#include "../include/ft_free_list.h"
#include "../include/ft_helper.h"
#include "../include/ft_printer.h"
#include "../include/ft_sort.h"
#include "../include/ft_str.h"
#include "../include/ft_walk.h"
#include "../include/ft_shell_escape.h"

#include "../libft/include/ft_fprintf.h"
#include "../libft/include/libft.h"

typedef struct {
    t_args *args;
    free_list fl;
    Arena *dirs_arena;
    Arena *temp_arena;
    t_array *dirs;
    t_array *files;
    t_array *entries;
    uint64_t max_len_links;
    uint64_t max_len_sizes;
} t_params;

static bool run_(const t_args *args, t_params *params, t_array *array,
                 int *exit_code);
static bool read_dir_(t_params *params, t_entry *path, int *exit_code);
static bool walk_recurssive_(t_params *params, free_list *fl);
static bool process_args_(t_params *params, t_array *array, int *exit_code);
static void clear_array_(t_array *array);
static void clear_temp_dir_(t_params *params);
static t_entry *create_entry_(free_list *fl, const t_entry *path, const struct dirent *dp);
static t_str *dup_str_arena_(Arena *arena, const t_str *src);
static t_entry *queue_dir_entry_(t_params *params, const t_entry *src,
                                 bool is_operand);
static int check_links(t_params *params, t_str *str, t_array *dir_entries,
                       struct stat *st, int *exit_code);
static t_str *join_paths_(free_list *fl, const t_str *lhs, const t_str *rhs);
static bool has_quoted_operands_(const t_array *array);
static void clean_up_(t_params *params);
static void print_err_(free_list *fl, t_str *str, int e, const char *prefix);

typedef enum { SINGLE_QUOTE = '\'', DOUBLE_QUOTE = '\"', SPACE = ' ' } t_quote;

void process(t_args *args, t_array *array, int *exit_code) {
    ASSERT_NOTNULL(args);
    ASSERT_NOTNULL(array);
    ASSERT_GE(array->cap, 1);
    ASSERT_GE(array->len, 1);
    ASSERT_NOTNULL(exit_code);

    const char *err_msg = NULL;
    unsigned char buffer[1024];
    t_params params = {0};
    params.args = args;
    fl_init(&params.fl, buffer, sizeof(buffer));
    params.dirs_arena = ArenaAlloc(ARENA_SIZE);
    if (!params.dirs_arena) {
        *exit_code = 2;
        return;
    }

    params.temp_arena = ArenaAlloc(UINT64_C(1024) * UINT64_C(1024));
    if (!params.temp_arena) {
        *exit_code = 2;
        clean_up_(&params);
        return;
    }

    ArenaSetAutoAlign(params.dirs_arena, 8);
    ArenaSetAutoAlign(params.temp_arena, 8);

    params.dirs = init_array(&params.fl, ARRAY_SIZE);
    params.files = init_array(&params.fl, ARRAY_SIZE);
    params.entries = init_array(&params.fl, ARRAY_SIZE);

    if (!params.dirs || !params.files || !params.entries) {
        *exit_code = 2;
        clean_up_(&params);
        return;
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

static bool run_(const t_args *args, t_params *params, t_array *array,
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
        reset_array(&params->fl, params->files);
    }

    while (params->dirs->len) {
        t_entry *dir_path = pop_array(params->dirs);

        if (!inserted_files_dirs_gap && printed_files && dir_path->is_operand) {
            write(STDOUT_FILENO, "\n", 1);
            inserted_files_dirs_gap = true;
        }

        if (!read_dir_(params, dir_path, exit_code)) {
            clear_temp_dir_(params);
            continue;
        }

        if (printed_dir) {
            write(STDOUT_FILENO, "\n", 1);
        }

        t_entry entry = {.name = dir_path->path};
        entry.quote = shell_quote_style(entry.name);
        if (entry.quote == '\0' && ft_strchr(entry.name->str, ':')) {
            entry.quote = '\'';
        }

        const t_entry *ent = print_dir_path ? &entry : NULL;
        printer(args, params->files, ent, true, 0, 0, false);
        printed_dir = true;
        clear_temp_dir_(params);
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
    // ASSERT_NOTNULL(params->fl);
    ASSERT_NOTNULL(params->dirs);
    ASSERT_NOTNULL(params->files);
    ASSERT_NOTNULL(array);
    ASSERT_NOTNULL(exit_code);

    const char *err_msg = NULL;
    struct stat st;
    unsigned char buffer[1024];
    free_list fl;
    fl_init(&fl, buffer, sizeof(buffer));
    t_array *dir_entries = init_array(&fl, ARRAY_SIZE);

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
            t_entry src = {.name = str, .path = str, .st = st};
            t_entry *dir_entry = queue_dir_entry_(params, &src, true);
            if (!dir_entry) {
                err_msg = "Failed to alloc dir entry";
                goto failed;
            }

            if (!append_array(dir_entries, dir_entry)) {
                err_msg = "Failed to append dir entry";
                goto failed;
            }
            continue;
        }

        t_entry *entry = fl_alloc(&params->fl, sizeof(*entry), 8);
        if (!entry) {
            err_msg = "Failed to alloc entry";
            goto failed;
        }

        entry->quote = shell_quote_style(str);
        entry->name = str;
        entry->path = str;
        entry->st = st;
        entry->is_escaped = false;
        entry->is_operand = false;
        if (params->args->list) {
            if (!get_file_info(&params->fl, entry)) {
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

static t_entry *create_entry_(free_list *fl, const t_entry *path, const struct dirent *dp) {
    ASSERT_NOTNULL(fl);
    ASSERT_NOTNULL(path);
    ASSERT_NOTNULL(dp);

    t_entry *entry = fl_alloc(fl, sizeof(*entry), 8);
    if (!entry) {
        goto failed;
    }

    entry->name = create_str(fl, dp->d_name);
    if (!entry->name) {
        goto failed;
    }

    entry->path = join_paths_(fl, path->path, entry->name);
    if (!entry->path) {
        goto failed;
    }

    entry->quote = shell_quote_style(entry->name);
    entry->is_escaped = false;
    entry->is_operand = false;

    return entry;
failed:
    return NULL;
}

static int check_links(t_params *params, t_str *str, t_array *dir_entries,
                       struct stat *st, int *exit_code) {
    const char *err_msg = NULL;
    if (lstat(str->str, st) == -1) {
        int e = errno;
        const char *prefix = "cannot access";
        print_err_(&params->fl, str, e, prefix);
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
    struct stat st_target;
    if (S_ISLNK(st->st_mode)) {
        if (stat(str->str, &st_target) == 0 && S_ISDIR(st_target.st_mode)) {
            is_dir_operand = true;
            st_dir = &st_target;
        }
    }

    if (is_dir_operand && !params->args->list) {
        t_entry src = {.name = str, .path = str, .st = *st_dir};
        t_entry *dir_entry = queue_dir_entry_(params, &src, true);
        if (!dir_entry) {
            err_msg = "Failed to alloc dir entry";
            goto failed;
        }

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
    // ASSERT_NOTNULL(params->dirs_fl);
    // ASSERT_NOTNULL(params->files_fl);
    // ASSERT_NOTNULL(params->entries_fl);
    // ASSERT_NOTNULL(params->scratch_fl);
    ASSERT_NOTNULL(params->dirs);
    ASSERT_NOTNULL(params->files);
    ASSERT_NOTNULL(path);
    ASSERT_NOTNULL(exit_code);

    errno = 0;
    DIR *d = opendir(path->path->str);
    if (!d) {
        int e = errno;
        const char *prefix = "cannot open directory";
        print_err_(&params->fl, path->path, e, prefix);
        *exit_code = (path->is_operand ? 2 : 1);
        return false;
    }

    clear_temp_dir_(params);

    const struct dirent *dp;
    free_list dir_fl;
    void *dir_buffer =
        ArenaPushNoZero(params->temp_arena, UINT64_C(1024) * UINT64_C(1024));
    if (!dir_buffer) {
        *exit_code = 2;
        return false;
    }

    fl_init(&dir_fl, dir_buffer, UINT64_C(1024) * UINT64_C(1024));

    while ((dp = readdir(d)) != NULL) {
        if (!params->args->all && dp->d_name[0] == '.' &&
            dp->d_name[1] != '/') {
            continue;
        }

        t_entry *entry = create_entry_(&dir_fl, path, dp);
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
            if (!get_file_info(&dir_fl, entry)) {
                goto failed;
            }
        }
        if (!append_array(params->files, entry)) {
            goto failed;
        }
    }
    closedir(d);
    d = NULL;

    return walk_recurssive_(params, &dir_fl);
failed:
    closedir(d);
    *exit_code = 2;
    return false;
}

static void clear_array_(t_array *array) {
    ASSERT_NOTNULL(array);

    while (array->len) {
        (void)pop_array(array);
    }
}

static void clear_temp_dir_(t_params *params) {
    ASSERT_NOTNULL(params);
    ASSERT_NOTNULL(params->files);
    ASSERT_NOTNULL(params->entries);
    ASSERT_NOTNULL(params->temp_arena);

    clear_array_(params->files);
    clear_array_(params->entries);
    ArenaClear(params->temp_arena);
}

static bool walk_recurssive_(t_params *params, free_list *fl) {
    ASSERT_NOTNULL(params);

    if (params->args->recursive && params->entries->len) {
        (void)fl;
        sort(params->entries, params->args->reverse, params->args->time);
        size_t index = params->entries->len;
        while (index > 0) {
            --index;
            const t_entry *entry = pop_array(params->entries);
            t_entry *dir_entry;
            if (!entry->name) {
                continue;
            }

            if (entry->name->str[0] == '.' &&
                (entry->name->str[1] == '\0' ||
                 (entry->name->str[1] == '.' && entry->name->str[2] == '\0'))) {
                continue;
            }

            dir_entry = queue_dir_entry_(params, entry, false);
            if (!dir_entry) {
                return false;
            }

            if (!append_array(params->dirs, dir_entry)) {
                return false;
            }
        }
    }

    return true;
}

static t_str *dup_str_arena_(Arena *arena, const t_str *src) {
    ASSERT_NOTNULL(arena);
    ASSERT_NOTNULL(src);
    ASSERT_NOTNULL(src->str);

    t_str *dst = ArenaPush(arena, sizeof(*dst));
    char *buf;
    if (!dst) {
        return NULL;
    }

    buf = ArenaPushNoZero(arena, src->cap);
    if (!buf) {
        return NULL;
    }

    *dst = *src;
    dst->str = buf;
    dst->pos = 0;
    ft_memcpy(dst->str, src->str, src->len + 1);
    return dst;
}

static t_entry *queue_dir_entry_(t_params *params, const t_entry *src,
                                 bool is_operand) {
    ASSERT_NOTNULL(params);
    ASSERT_NOTNULL(params->dirs_arena);
    ASSERT_NOTNULL(src);
    ASSERT_NOTNULL(src->path);

    t_entry *entry = ArenaPush(params->dirs_arena, sizeof(*entry));
    if (!entry) {
        return NULL;
    }

    *entry = *src;
    entry->name =
        src->name ? dup_str_arena_(params->dirs_arena, src->name) : NULL;
    if (src->name && !entry->name) {
        return NULL;
    }

    entry->path = dup_str_arena_(params->dirs_arena, src->path);
    if (!entry->path) {
        return NULL;
    }

    entry->quote = '\0';
    entry->info = NULL;
    entry->is_operand = is_operand;
    return entry;
}

static t_str *join_paths_(free_list *fl, const t_str *lhs, const t_str *rhs) {
    ASSERT_NOTNULL(fl);
    ASSERT_NOTNULL(lhs);
    ASSERT_NOTNULL(rhs);

    const size_t new_len = lhs->len + 1 + rhs->len + 1;
    t_str *fullname = init_str(fl, new_len);
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

static bool has_quoted_operands_(const t_array *array) {
    ASSERT_NOTNULL(array);

    for (uint64_t index = 0; index < array->len; ++index) {
        const t_str *operand = array->data[index];
        if (!operand) {
            continue;
        }

        if (has_shell_quote_char(operand)) {
            return true;
        }
    }

    return false;
}

static void clean_up_(t_params *params) {
    ASSERT_NOTNULL(params);

    if (params->dirs_arena) {
        ArenaRelease(params->dirs_arena);
        params->dirs_arena = NULL;
    }

    if (params->temp_arena) {
        ArenaRelease(params->temp_arena);
        params->temp_arena = NULL;
    }

    fl_free_all(&params->fl);
}

static void print_err_(free_list *fl, t_str *str, int e, const char *prefix) {
    const char *msg = strerror(e);
    const char quote = shell_quote_style(str);

    if (quote != '\0') {
        t_str *new_str = shell_escape_str(fl, str, quote);
        if (new_str) {
            ft_fprintf(STDERR_FILENO, "ft_ls: %s %s: %s\n", prefix,
                       new_str->str, msg);
            return;
        }
    }

    ft_fprintf(STDERR_FILENO, "ft_ls: %s '%s': %s\n", prefix, str->str, msg);
}
