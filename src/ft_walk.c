#include <dirent.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../include/ft_arena.h"
#include "../include/ft_array.h"
#include "../include/ft_entry.h"
#include "../include/ft_free_list.h"
#include "../include/ft_helper.h"
#include "../include/ft_printer.h"
#include "../include/ft_shell_escape.h"
#include "../include/ft_sort.h"
#include "../include/ft_str.h"
#include "../include/ft_walk.h"

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

static bool run_(t_args *args, t_params *params, t_array *array,
                 int *exit_code);
static bool read_dir_(t_params *params, t_entry *path, int *exit_code);
static bool walk_recurssive_(t_params *params);
static bool process_args_(t_params *params, t_array *array, int *exit_code);
static void clear_temp_dir_(t_params *params);
static void free_entry_array_(t_params *params, t_array *array);
static t_str *dup_dir_str_(Arena *arena, const t_str *src);
static t_str *join_dir_path_(Arena *arena, const t_str *lhs, const t_str *rhs);
static bool ensure_entry_path_(Arena *arena, t_entry *entry);
static mode_t mode_from_dtype_(unsigned char dtype);
static bool entry_needs_lstat_(const t_args *args, unsigned char dtype);
static t_entry *queue_dir_entry_(t_params *params, const t_entry *src,
                                 bool is_operand);
static int check_links_(t_params *params, t_str *str, t_array *dir_entries,
                        struct stat *st, int *exit_code);
static bool has_quoted_operands_(const t_array *array);
static void clean_up_(t_params *params);
static void print_err_(free_list *fl, t_str *str, int e, const char *prefix);

typedef enum { SINGLE_QUOTE = '\'', DOUBLE_QUOTE = '\"', SPACE = ' ' } t_quote;

void process(t_args *args, t_array *array, int *exit_code) {
    const char *err_msg = NULL;
    unsigned char buffer[FL_DEFAULT_SIZE];
    t_params params = {0};

    params.args = args;
    fl_init(&params.fl, buffer, sizeof(buffer));
    params.dirs_arena = arena_alloc(ARENA_SIZE);
    if (!params.dirs_arena) {
        *exit_code = 2;
        goto cleanup;
    }

    params.temp_arena = arena_alloc(ARENA_SIZE);
    if (!params.temp_arena) {
        *exit_code = 2;
        goto cleanup;
    }

    arena_auto_align(params.dirs_arena, 8);
    arena_auto_align(params.temp_arena, 8);

    params.dirs = init_array(&params.fl, ARRAY_SIZE);
    params.files = init_array(&params.fl, ARRAY_SIZE);
    params.entries = init_array(&params.fl, ARRAY_SIZE);

    if (!params.dirs || !params.files || !params.entries) {
        *exit_code = 2;
        goto cleanup;
    }

    if (!run_(args, &params, array, exit_code)) {
        goto cleanup;
    }
cleanup:
    if (err_msg) {
        ft_fprintf(STDERR_FILENO, "Error: %s\n", err_msg);
    }

    clean_up_(&params);
}

static bool run_(t_args *args, t_params *params, t_array *array,
                 int *exit_code) {
    const char *err_msg = NULL;
    if (!process_args_(params, array, exit_code)) {
        goto failed;
    }

    const bool has_quoted_operands = has_quoted_operands_(array);
    bool printed_files = false;
    t_ps ps = {.args = args,
               .array = params->files,
               .dir_entry = NULL,
               .print_total = false,
               .min_len_links = params->max_len_links,
               .min_len_sizes = params->max_len_sizes,
               .quote_padding = has_quoted_operands};
    if (params->files->len) {
        printer(&ps);
        printed_files = true;
        free_entry_array_(params, params->files);
    }

    bool printed_dir = false;
    bool inserted_files_dirs_gap = false;
    const bool print_dir_path = args->recursive || array->len > 1;
    ps.print_total = true;
    ps.min_len_links = 0;
    ps.min_len_sizes = 0;
    ps.quote_padding = false;
    while (params->dirs->len) {
        t_entry *dir_path = pop_array(params->dirs);

        if (!inserted_files_dirs_gap && printed_files && dir_path->is_operand) {
            if (write(STDOUT_FILENO, "\n", 1) < 0) {
                goto failed;
            }
            inserted_files_dirs_gap = true;
        }

        if (!read_dir_(params, dir_path, exit_code)) {
            clear_temp_dir_(params);
            continue;
        }

        if (printed_dir) {
            if (write(STDOUT_FILENO, "\n", 1) < 0) {
                goto failed;
            }
        }

        t_entry entry = {.name = dir_path->path,
                         .quote = dir_path->quote,
                         .display_len = dir_path->display_len,
                         .padded_display_len = dir_path->padded_display_len};

        t_entry *ent = print_dir_path ? &entry : NULL;
        ps.array = params->files;
        ps.dir_entry = ent;
        printer(&ps);
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
    const char *err_msg = NULL;
    bool ok = false;
    struct stat st;
    unsigned char buffer[FL_DEFAULT_SIZE];
    free_list fl;
    fl_init(&fl, buffer, sizeof(buffer));
    t_array *dir_entries = init_array(&fl, ARRAY_SIZE);

    if (!dir_entries) {
        err_msg = "Failed to init dir entries";
        goto failed;
    }

    for (uint64_t index = 0; index < array->len; ++index) {
        t_str *str = array->data[index];

        int state = check_links_(params, str, dir_entries, &st, exit_code);
        if (state == 0) {
            continue;
        } else if (state < 0) {
            goto failed;
        }

        if (S_ISDIR(st.st_mode)) {
            t_entry src = {
                .name = str,
                .path = str,
                .path_has_colon = ft_memchr(str->str + str->pos, ':',
                                            (size_t)str->len) != NULL,
                .st = st,
            };
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

        *entry = (t_entry){0};
        entry->name = dup_str(&params->fl, str);
        if (!entry->name) {
            err_msg = "Failed to duplicate entry name";
            free_entry(&params->fl, entry);
            goto failed;
        }

        entry->path = entry->name;
        entry->path_has_colon =
            ft_memchr(str->str + str->pos, ':', (size_t)str->len) != NULL;
        entry->st = st;
        entry->is_escaped = false;
        entry->is_operand = false;
        init_entry_display(entry);
        if (params->args->list) {
            if (!get_file_info(&params->fl, entry)) {
                err_msg = "Failed to get file info";
                free_entry(&params->fl, entry);
                goto failed;
            }
        }

        if (!append_array(params->files, entry)) {
            err_msg = "Failed to append file entry";
            free_entry(&params->fl, entry);
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

    ok = true;
cleanup:
    fl_free_all(&fl);
    return ok;
failed:
    ft_fprintf(STDERR_FILENO, "Error: %s\n", err_msg);
    *exit_code = 2;
    goto cleanup;
}

static int check_links_(t_params *params, t_str *str, t_array *dir_entries,
                        struct stat *st, int *exit_code) {
    int e = 0;
    const char *err_msg = NULL;
    if (lstat(str->str, st) == -1) {
        e = errno;
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

        Arena_Mark arena_mark = arena_get_mark(params->temp_arena);
        char *buf = arena_push(params->temp_arena, str->len + 1);
        if (readlink(str->str, buf, str->len) < 0) {
            e = errno;
            *exit_code = 2;
            if (params->args->list) {
                print_err_(&params->fl, str, e, "cannot read symbolic link");
            } else {
                print_err_(&params->fl, str, e, "cannot access");
                arena_pop_to_mark(params->temp_arena, arena_mark);
                return 0;
            }
        }
        arena_pop_to_mark(params->temp_arena, arena_mark);
    }

    if (is_dir_operand && !params->args->list) {
        t_entry src = {
            .name = str,
            .path = str,
            .path_has_colon =
                ft_memchr(str->str + str->pos, ':', (size_t)str->len) != NULL,
            .st = *st_dir,
        };
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
    errno = 0;
    bool ok = false;
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

    while ((dp = readdir(d)) != NULL) {
        const unsigned char dtype = dp->d_type;
        const bool need_lstat = entry_needs_lstat_(params->args, dtype);
        if (!params->args->all && dp->d_name[0] == '.' &&
            dp->d_name[1] != '/') {
            continue;
        }

        t_entry *entry = new_entry(params->temp_arena, path, dp);
        if (!entry) {
            goto cleanup;
        }

        if (need_lstat) {
            if (!ensure_entry_path_(params->temp_arena, entry)) {
                goto cleanup;
            }
            if (lstat(entry->path->str, &entry->st) == -1) {
                *exit_code = 2;
                continue;
            }
        } else {
            entry->st.st_mode = mode_from_dtype_(dtype);
        }

        if (params->args->list) {
            if (!get_file_info_arena(params->temp_arena, entry)) {
                goto cleanup;
            }
        }

        if (!append_array(params->files, entry)) {
            goto cleanup;
        }

        if (params->args->recursive && S_ISDIR(entry->st.st_mode)) {
            if (!append_array(params->entries, entry)) {
                (void)pop_array(params->files);
                goto cleanup;
            }
        }
    }

    ok = walk_recurssive_(params);
cleanup:
    if (d) {
        closedir(d);
    }

    if (!ok) {
        *exit_code = 2;
    }

    return ok;
}

static void clear_temp_dir_(t_params *params) {
    clear_array(params->entries);
    clear_array(params->files);
    arena_clear(params->temp_arena);
}

static void free_entry_array_(t_params *params, t_array *array) {
    while (array->len) {
        t_entry *entry = pop_array(array);
        free_entry(&params->fl, entry);
    }
}

static t_str *join_dir_path_(Arena *arena, const t_str *lhs, const t_str *rhs) {
    const bool need_slash = lhs->len == 0 || lhs->str[lhs->len - 1] != '/';
    const uint64_t total_len = lhs->len + rhs->len + (need_slash ? 1U : 0U);
    t_str *path = init_str_arena(arena, total_len);

    if (!path) {
        return NULL;
    }

    ft_memcpy(path->str, lhs->str + lhs->pos, (size_t)lhs->len);
    path->len = lhs->len;
    if (need_slash) {
        path->str[path->len++] = '/';
    }

    ft_memcpy(path->str + path->len, rhs->str + rhs->pos, (size_t)rhs->len);
    path->len += rhs->len;
    path->str[path->len] = '\0';
    return path;
}

static bool ensure_entry_path_(Arena *arena, t_entry *entry) {
    if (entry->path) {
        return true;
    }

    if (!entry->parent_path || !entry->name) {
        return false;
    }

    entry->path = join_dir_path_(arena, entry->parent_path, entry->name);
    return entry->path != NULL;
}

static mode_t mode_from_dtype_(unsigned char dtype) {
    switch (dtype) {
        case DT_BLK: return S_IFBLK;
        case DT_CHR: return S_IFCHR;
        case DT_DIR: return S_IFDIR;
        case DT_FIFO: return S_IFIFO;
        case DT_LNK: return S_IFLNK;
        case DT_REG: return S_IFREG;
        case DT_SOCK: return S_IFSOCK;
        default: return 0;
    }
}

static bool entry_needs_lstat_(const t_args *args, unsigned char dtype) {
    if (args->list || args->time) {
        return true;
    }

    return mode_from_dtype_(dtype) == 0;
}

static bool walk_recurssive_(t_params *params) {
    if (params->args->recursive && params->entries->len) {
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
                free_entry(&params->fl, dir_entry);
                return false;
            }
        }
    }

    return true;
}

static t_str *dup_dir_str_(Arena *arena, const t_str *src) {
    t_str *dst = init_str_arena(arena, src->len);
    if (!dst) {
        return NULL;
    }

    const char *src_bytes = src->str + src->pos;
    ft_memcpy(dst->str, src_bytes, (size_t)src->len);
    dst->len = src->len;
    dst->str[dst->len] = '\0';
    return dst;
}

static t_entry *queue_dir_entry_(t_params *params, const t_entry *src,
                                 bool is_operand) {
    Arena_Mark mark = arena_get_mark(params->dirs_arena);
    t_shell_scan scan;
    t_entry *entry = arena_push(params->dirs_arena, sizeof(*entry));
    if (!entry) {
        return NULL;
    }

    *entry = (t_entry){0};
    entry->name =
        src->name ? dup_dir_str_(params->dirs_arena, src->name) : NULL;
    if (src->name && !entry->name) {
        goto failed;
    }

    if (src->path == NULL) {
        if (!src->parent_path || !entry->name) {
            goto failed;
        }
        entry->path =
            join_dir_path_(params->dirs_arena, src->parent_path, entry->name);
    } else if (src->path == src->name) {
        entry->path = entry->name;
    } else {
        entry->path = dup_dir_str_(params->dirs_arena, src->path);
    }

    if (!entry->path) {
        goto failed;
    }

    shell_scan_str(entry->path, &scan);
    entry->st = src->st;
    entry->quote = scan.quote;
    entry->path_has_colon = src->path_has_colon;
    entry->parent_path = NULL;
    entry->display_len = scan.display_len;
    entry->padded_display_len = scan.padded_display_len;
    if (entry->quote == '\0' && entry->path_has_colon) {
        entry->quote = '\'';
        entry->display_len = entry->path->len + 2;
        entry->padded_display_len = entry->display_len;
    }
    entry->is_escaped = false;
    entry->info = NULL;
    entry->is_operand = is_operand;
    return entry;
failed:
    arena_pop_to_mark(params->dirs_arena, mark);
    return NULL;
}

static bool has_quoted_operands_(const t_array *array) {
    for (uint64_t index = 0; index < array->len; ++index) {
        const t_str *operand = array->data[index];
        t_shell_scan scan;
        if (!operand) {
            continue;
        }

        shell_scan_str(operand, &scan);
        if (scan.quote != '\0') {
            return true;
        }
    }

    return false;
}

static void clean_up_(t_params *params) {
    if (params->dirs_arena) {
        arena_release(params->dirs_arena);
    }

    if (params->temp_arena) {
        arena_release(params->temp_arena);
    }

    fl_free_all(&params->fl);
}

static void print_err_(free_list *fl, t_str *str, int e, const char *prefix) {
    const char *msg = strerror(e);
    t_shell_scan scan;

    shell_scan_str(str, &scan);

    if (scan.quote != '\0') {
        t_str *new_str = shell_escape_str(fl, str, scan.quote);
        if (new_str) {
            ft_fprintf(STDERR_FILENO, "ft_ls: %s %s: %s\n", prefix,
                       new_str->str, msg);
            free_str(fl, new_str);
            return;
        }
    }

    ft_fprintf(STDERR_FILENO, "ft_ls: %s '%s': %s\n", prefix, str->str, msg);
}
