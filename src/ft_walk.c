#include <dirent.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../include/ft_arena.h"
#include "../include/ft_array.h"
#include "../include/ft_entry.h"
#include "../include/ft_free_list.h"
#include "../include/ft_helper.h"
#include "../include/ft_printer.h"
#include "../include/ft_printer_helper.h"
#include "../include/ft_shell_escape.h"
#include "../include/ft_sort.h"
#include "../include/ft_str.h"
#include "../include/ft_walk.h"

#include "../libft/include/ft_fprintf.h"
#include "../libft/include/libft.h"

typedef struct {
    t_args *args;
    free_list fl;
    Arena *temp_arena;
    t_sort_scratch sort_scratch;
    t_array *dirs;
    t_array *files;
    char out_buf[OUTPUT_BUFFER_CAP];
    t_str out;
    t_list_stats file_stats;
    t_list_stats dir_stats;
    uint64_t max_len_links;
    uint64_t max_len_sizes;
    bool output_failed;
} t_params;

static bool run_(t_params *params, const t_array *array, int *exit_code);
static bool read_dir_(t_params *params, t_entry *path, int *exit_code);
static bool walk_recurssive_(t_params *params);
static bool process_args_(t_params *params, t_array *array, int *exit_code);
static void clear_temp_dir_(t_params *params);
static void free_entry_array_(t_params *params, t_array *array);
static mode_t mode_from_dtype_(unsigned char dtype);
static bool entry_needs_lstat_(const t_args *args, unsigned char dtype);
static int check_links_(t_params *params, t_str *str, t_array *dir_entries,
                        struct stat *st, t_str **symlink, int *exit_code);
static bool has_quoted_operands_(const t_array *array);
static void clean_up_(t_params *params);
static bool print_err_(t_params *params, t_str *str, int e, const char *prefix);
static void set_entry_(t_entry *entry, t_str *str, const struct stat *st);

void process(t_args *args, t_array *array, int *exit_code) {
    unsigned char buffer[FL_DEFAULT_SIZE];
    t_params params = {0};

    params.args = args;
    fl_init(&params.fl, buffer, sizeof(buffer));
    params.out = (t_str){.str = params.out_buf,
                         .cap = sizeof(params.out_buf),
                         .len = 0,
                         .pos = 0};
    params.out.str[0] = '\0';
    params.temp_arena = arena_alloc(ARENA_SIZE);
    if (!params.temp_arena) {
        *exit_code = 2;
        goto cleanup;
    }

    arena_auto_align(params.temp_arena, 8);

    params.dirs = init_array(&params.fl, ARRAY_SIZE);
    params.files = init_array(&params.fl, ARRAY_SIZE);

    if (!params.dirs || !params.files) {
        *exit_code = 2;
        goto cleanup;
    }

    if (!process_args_(&params, array, exit_code)) {
        *exit_code = 2;
        goto cleanup;
    }

    if (!run_(&params, array, exit_code)) {
        *exit_code = 2;
        goto cleanup;
    }
cleanup:
    clean_up_(&params);
}

static bool run_(t_params *params, const t_array *array, int *exit_code) {
    const bool has_quoted_operands = has_quoted_operands_(array);
    bool printed_files = false;
    params->out.len = 0;
    params->out.pos = 0;
    params->out.str[0] = '\0';
    params->output_failed = false;

    t_ps ps = {.array = params->files,
               .dir_entry = NULL,
               .buffer = &params->out,
               .stats = params->file_stats,
               .print_total = false,
               .min_len_links = params->max_len_links,
               .min_len_sizes = params->max_len_sizes,
               .quote_padding = has_quoted_operands};
    if (params->files->len) {
        if (!sort(&params->sort_scratch, params->files, params->args->reverse,
             params->args->time)) {
            free_entry_array_(params, params->files);
            return false;
        }
        printer(&ps, params->args->list);
        printed_files = true;
        free_entry_array_(params, params->files);
    }

    bool printed_dir = false;
    bool inserted_files_dirs_gap = false;
    const bool print_dir_path = params->args->recursive || array->len > 1;
    ps.print_total = true;
    ps.min_len_links = 0;
    ps.min_len_sizes = 0;
    ps.quote_padding = false;
    t_entry *dir_path = NULL;
    while (params->dirs->len) {
        dir_path = pop_array(params->dirs);

        if (!inserted_files_dirs_gap && printed_files && dir_path->is_operand) {
            if (!put_mem(ps.buffer, "\n", 1)) {
                goto error;
            }
            inserted_files_dirs_gap = true;
        }

        if (!read_dir_(params, dir_path, exit_code)) {
            clear_temp_dir_(params);
            if (params->output_failed) {
                goto error;
            }
            free_entry(&params->fl, dir_path);
            continue;
        }

        if (!sort(&params->sort_scratch, params->files, params->args->reverse,
             params->args->time)) {
            goto error;
        }

        if (!walk_recurssive_(params)) {
            goto error;
        }

        if (printed_dir) {
            if (!put_mem(ps.buffer, "\n", 1)) {
                goto error;
            }
        }

        t_entry entry = {.name = dir_path->path,
                         .quote = dir_path->quote,
                         .display_len = dir_path->display_len,
                         .padded_display_len = dir_path->padded_display_len};

        t_entry *ent = print_dir_path ? &entry : NULL;
        ps.array = params->files;
        ps.dir_entry = ent;
        ps.stats = params->dir_stats;
        printer(&ps, params->args->list);
        printed_dir = true;
        clear_temp_dir_(params);
        free_entry(&params->fl, dir_path);
    }

    return flush_str(&params->out);
error:
    free_entry(&params->fl, dir_path);
    flush_str(&params->out);
    return false;
}

static bool process_args_(t_params *params, t_array *array, int *exit_code) {
    bool ok = false;
    unsigned char buffer[FL_DEFAULT_SIZE];
    free_list fl;
    fl_init(&fl, buffer, sizeof(buffer));
    t_array *dir_entries = init_array(&fl, ARRAY_SIZE);
    if (!dir_entries) {
        goto failed;
    }

    struct stat st;
    const t_alloc alloc = {.kind = ALLOC_FL, .as.fl = &params->fl};
    for (uint64_t index = 0; index < array->len; ++index) {
        t_str *str = array->data[index];
        t_str *symlink = NULL;

        int state =
            check_links_(params, str, dir_entries, &st, &symlink, exit_code);
        if (state == 0) {
            if (symlink) {
                free_str(&params->fl, symlink);
            }
            continue;
        } else if (state < 0) {
            goto failed;
        }

        if (S_ISDIR(st.st_mode)) {
            t_entry src = {0};
            set_entry_(&src, str, &st);
            t_entry *dir_entry = dup_dir_entry(&alloc, &src, true);
            if (!dir_entry) {
                goto failed;
            }

            if (!append_array(dir_entries, dir_entry)) {
                goto failed;
            }
            continue;
        }

        t_entry *entry = fl_alloc(&params->fl, sizeof(*entry), 8);
        if (!entry) {
            goto failed;
        }

        *entry = (t_entry){0};
        entry->name = dup_str(&alloc, str);
        if (!entry->name) {
            free_entry(&params->fl, entry);
            goto failed;
        }

        entry->path = entry->name;
        entry->path_has_colon =
            ft_memchr(str->str + str->pos, ':', (size_t)str->len) != NULL;
        entry->st = st;
        entry->symlink = symlink;
        entry->symlink_ready = symlink != NULL;
        entry->is_operand = false;
        init_entry_display(entry);
        if (params->args->list) {
            if (!get_file_info(&alloc, entry)) {
                free_entry(&params->fl, entry);
                goto failed;
            }

            update_list_stats(&params->file_stats, entry);
        }

        if (!append_array(params->files, entry)) {
            free_entry(&params->fl, entry);
            goto failed;
        }
    }

    if (dir_entries->len) {
        if (!sort(&params->sort_scratch, dir_entries, params->args->reverse,
             params->args->time)) {
            goto failed;
        }

        while (dir_entries->len) {
            t_entry *entry = pop_array(dir_entries);
            entry->is_operand = true;
            if (!append_array(params->dirs, entry)) {
                goto failed;
            }
        }
    }

    ok = true;
cleanup:
    fl_free_all(&fl);
    return ok;
failed:
    *exit_code = 2;
    goto cleanup;
}

static int check_links_(t_params *params, t_str *str, t_array *dir_entries,
                        struct stat *st, t_str **symlink, int *exit_code) {
    int e;

    *symlink = NULL;
    if (lstat(str->str, st) == -1) {
        e = errno;
        const char *prefix = "cannot access";
        if (!print_err_(params, str, e, prefix)) {
            return -1;
        }
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
    const t_alloc alloc = {.kind = ALLOC_FL, .as.fl = &params->fl};
    if (S_ISLNK(st->st_mode)) {
        if (stat(str->str, &st_target) == 0 && S_ISDIR(st_target.st_mode)) {
            is_dir_operand = true;
            st_dir = &st_target;
        }

        *symlink = read_symlink_target(&alloc, str, (uint64_t)st->st_size, &e);
        if (!*symlink) {
            return -1;
        }

        if (e != 0) {
            *exit_code = 2;
            if (params->args->list) {
                if (!print_err_(params, str, e, "cannot read symbolic link")) {
                    return -1;
                }
            } else {
                if (!print_err_(params, str, e, "cannot access")) {
                    return -1;
                }
                return 0;
            }
        }
    }

    if (is_dir_operand && !params->args->list) {
        t_entry src = {0};
        set_entry_(&src, str, st_dir);
        t_entry *dir_entry = dup_dir_entry(&alloc, &src, true);
        if (!dir_entry) {
            return -1;
        }

        if (!append_array(dir_entries, dir_entry)) {
            return -1;
        }
        return 0;
    }

    return 1;
}

static bool read_dir_(t_params *params, t_entry *path, int *exit_code) {
    errno = 0;
    bool ok = false;
    DIR *d = opendir(path->path->str);
    if (!d) {
        int e = errno;
        if (!print_err_(params, path->path, e, "cannot open directory")) {
            params->output_failed = true;
        }
        *exit_code = (path->is_operand ? 2 : 1);
        return false;
    }

    clear_temp_dir_(params);
    const struct dirent *dp;

    const t_alloc alloc = {.kind = ALLOC_ARENA, .as.arena = params->temp_arena};
    while ((dp = readdir(d)) != NULL) {
        const unsigned char dtype = dp->d_type;
        const bool need_lstat = entry_needs_lstat_(params->args, dtype);
        if (!params->args->all && dp->d_name[0] == '.' &&
            dp->d_name[1] != '/') {
            continue;
        }

        t_entry *entry = new_entry(&alloc, path, dp);
        if (!entry) {
            goto cleanup;
        }

        if (need_lstat) {
            if (!ensure_entry_path(&alloc, entry)) {
                goto cleanup;
            }

            if (lstat(entry->path->str, &entry->st) == -1) {
                *exit_code = 2;
                continue;
            }

            if (params->args->list && S_ISLNK(entry->st.st_mode)) {
                entry->symlink = read_symlink_target(
                    &alloc, entry->path, (uint64_t)entry->st.st_size, NULL);
                if (!entry->symlink) {
                    goto cleanup;
                }

                entry->symlink_ready = true;
            }
        } else {
            entry->st.st_mode = mode_from_dtype_(dtype);
        }

        if (params->args->list) {
            if (!get_file_info(&alloc, entry)) {
                goto cleanup;
            }

            update_list_stats(&params->dir_stats, entry);
        }

        if (!append_array(params->files, entry)) {
            goto cleanup;
        }
    }

    ok = true;
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
    clear_array(params->files);
    arena_clear(params->temp_arena);
    params->dir_stats = (t_list_stats){0};
}

static void free_entry_array_(t_params *params, t_array *array) {
    while (array->len) {
        t_entry *entry = pop_array(array);
        free_entry(&params->fl, entry);
    }
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
    if (params->args->recursive && params->files->len) {
        const t_alloc alloc = {.kind = ALLOC_FL, .as.fl = &params->fl};
        uint64_t index = params->files->len;
        while (index > 0) {
            --index;
            const t_entry *entry = params->files->data[index];
            t_entry *dir_entry;
            if (!entry || !entry->name || !S_ISDIR(entry->st.st_mode)) {
                continue;
            }

            if (ft_strncmp(entry->name->str, ".", entry->name->len) == 0 ||
                ft_strncmp(entry->name->str, "..", entry->name->len) == 0) {
                continue;
            }

            dir_entry = dup_dir_entry(&alloc, entry, false);
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

static bool has_quoted_operands_(const t_array *array) {
    for (uint64_t index = 0; index < array->len; ++index) {
        const t_str *operand = array->data[index];
        if (!operand) {
            continue;
        }

        t_shell_scan scan;
        shell_scan_str(operand, &scan);
        if (scan.quote != '\0') {
            return true;
        }
    }

    return false;
}

static void clean_up_(t_params *params) {
    if (params->temp_arena) {
        arena_release(params->temp_arena);
    }

    free((void *)params->sort_scratch.data);
    fl_free_all(&params->fl);
}

static bool print_err_(t_params *params, t_str *str, int e,
                       const char *prefix) {
    const char *msg = strerror(e);
    t_shell_scan scan;
    free_list *fl = &params->fl;

    if (!flush_str(&params->out)) {
        params->output_failed = true;
        return false;
    }

    shell_scan_str(str, &scan);
    if (scan.quote != '\0') {
        t_str *new_str = shell_escape_str(fl, str, scan.quote);
        if (new_str) {
            ft_fprintf(STDERR_FILENO, "ft_ls: %s %s: %s\n", prefix,
                       new_str->str, msg);
            free_str(fl, new_str);
            return true;
        }
    }

    ft_fprintf(STDERR_FILENO, "ft_ls: %s '%s': %s\n", prefix, str->str, msg);
    return true;
}

static void set_entry_(t_entry *entry, t_str *str, const struct stat *st) {
    entry->name = str;
    entry->path = str;
    entry->path_has_colon =
        ft_memchr(str->str + str->pos, ':', (size_t)str->len) != NULL,
    entry->st = *st;
}
