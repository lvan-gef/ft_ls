#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "./ft_arena.h"
#include "./ft_array.h"
#include "./ft_ls.h"
#include "./ft_out.h"
#include "./ft_parse.h"
#include "./ft_printer.h"
#include "./ft_printer_helper.h"
#include "./ft_shell_escape.h"
#include "./ft_sort.h"
#include "./ft_str.h"
#include "./ft_symlink.h"
#include "./ft_utils.h"
#include "./ft_walk.h"
#include "./ft_walk_entry.h"

typedef struct {
    const t_args *args;
    t_arena temp_arena;
    t_array dir_queue;
    t_array operand_files;
    t_array current_entries;
    t_out out;
    t_print_ctx ctx;
    bool quote_padding;
    bool dir_quoted;
    int exit_code;
} t_params;

static bool run_listing_(t_params *params, const t_array *array);
static bool print_operand_files_(t_params *params);
static bool process_queue_(t_params *params, const t_array *array);
static bool load_directory_entries_(t_params *params, DIR *d,
                                    const t_str *dir_path);
static bool queue_recursive_dirs_(t_params *params, const t_str *parent_path);
static bool collect_operands_(t_params *params, const t_array *array);
static bool collect_operand_(t_params *params, const t_str *str);
static bool add_operand_(t_params *params, t_array *dst, const t_str *str,
                         const struct stat *st);
static void quote_str_(t_params *params, const t_str *str);
static void clear_directory_entries_(t_params *params);
static void cleanup_process_(t_params *params);
static void print_error_(t_out *out, const t_str *path, int e,
                         const char *prefix);
static mode_t dtype_to_mode_(unsigned char dtype);
static void set_exit_(t_params *params, int code);

int process(const t_args *args, const t_array *array) {
    static char out_buf[OUTPUT_BUFFER_CAP];
    t_params params = {0};
    int exit_code = 0;

    params.args = args;
    out_init(&params.out, out_buf, sizeof(out_buf), STDOUT_FILENO);
    if (!arena_init(&params.temp_arena, ARENA_SIZE)) {
        set_exit_(&params, 2);
        goto cleanup;
    }

    if (!array_init(&params.dir_queue, ARRAY_SIZE) ||
        !array_init(&params.operand_files, ARRAY_SIZE) ||
        !array_init(&params.current_entries, ARRAY_SIZE)) {
        set_exit_(&params, 2);
        goto cleanup;
    }

    if (!collect_operands_(&params, array)) {
        set_exit_(&params, 2);
        goto cleanup;
    }

    if (!run_listing_(&params, array)) {
        set_exit_(&params, 2);
    }
cleanup:
    exit_code = params.exit_code;
    cleanup_process_(&params);
    return exit_code;
}

static bool run_listing_(t_params *params, const t_array *array) {
    params->ctx = (t_print_ctx){.args = params->args,
                                .out = &params->out,
                                .arena = &params->temp_arena,
                                .is_tty = isatty(STDOUT_FILENO),
                                .term_size = get_terminal_width()};

    const bool ok =
        print_operand_files_(params) && process_queue_(params, array);
    out_flush(&params->out);
    return ok && !params->out.failed;
}

static bool print_operand_files_(t_params *params) {
    if (!params->operand_files.len) {
        return true;
    }

    bool ok = sort(&params->operand_files, params->args, params->args->reverse);
    if (ok) {
        const t_print_request req = {.ctx = &params->ctx,
                                     .entries = &params->operand_files,
                                     .list_width_context = &params->dir_queue,
                                     .quoted = params->quote_padding};
        ok = printer(&req);
        if (ok && params->dir_queue.len) {
            out_put(&params->out, "\n", 1);
        }
    }

    array_clear_with(&params->operand_files, entry_del);
    return ok;
}

static bool process_queue_(t_params *params, const t_array *array) {
    bool printed_dir = false;
    const bool print_dir_path = params->args->recursive || array->len > 1;
    t_entry *dir_path = NULL;

    if (params->args->unsort) {
        array_reverse(&params->dir_queue);
    }

    while (params->dir_queue.len && !params->out.failed) {
        dir_path = array_pop(&params->dir_queue);

        DIR *d = opendir(dir_path->path->str);
        if (!d) {
            print_error_(&params->out, dir_path->path, errno,
                         "cannot open directory");
            set_exit_(params, dir_path->is_operand ? 2 : 1);
            entry_free(dir_path);
            dir_path = NULL;
            continue;
        }

        const bool ok = load_directory_entries_(params, d, dir_path->path);
        closedir(d);
        if (!ok) {
            goto error;
        }

        if (!sort(&params->current_entries, params->args,
                  params->args->reverse)) {
            goto error;
        }

        if (!queue_recursive_dirs_(params, dir_path->path)) {
            goto error;
        }

        if (printed_dir) {
            out_put(&params->out, "\n", 1);
        }

        const t_print_request req = {.ctx = &params->ctx,
                                     .entries = &params->current_entries,
                                     .dir_header =
                                         print_dir_path ? dir_path->path : NULL,
                                     .quoted = params->dir_quoted,
                                     .print_total = true};
        if (!printer(&req)) {
            goto error;
        }

        printed_dir = true;
        clear_directory_entries_(params);
        entry_free(dir_path);
        dir_path = NULL;
    }

    return true;
error:
    entry_free(dir_path);
    return false;
}

static bool collect_operands_(t_params *params, const t_array *array) {
    for (size_t index = 0; index < array->len; ++index) {
        char *arg = array->data[index];
        const t_str str = {.str = arg, .len = ft_strlen(arg)};
        if (!collect_operand_(params, &str)) {
            return false;
        }
    }

    if (!sort(&params->dir_queue, params->args, !params->args->reverse)) {
        return false;
    }

    return true;
}

static bool collect_operand_(t_params *params, const t_str *str) {
    const t_args *args = params->args;
    struct stat st;
    struct stat target;
    const struct stat *dir_st = &st;

    if (lstat(str->str, &st) < 0) {
        print_error_(&params->out, str, errno, "cannot access");
        set_exit_(params, 2);
        quote_str_(params, str);
        return true;
    }

    bool is_dir = S_ISDIR(st.st_mode);
    if (S_ISLNK(st.st_mode) && (args->list || !args->directory)) {
        if (!args->list && !stat(str->str, &target) &&
            S_ISDIR(target.st_mode)) {
            is_dir = true;
            dir_st = &target;
        }

        int e = 0;
        if (!read_symlink(&params->temp_arena, str, (size_t)st.st_size, &e)) {
            return false;
        }

        if (e) {
            set_exit_(params, 2);
            print_error_(&params->out, str, e,
                         args->list ? "cannot read symbolic link"
                                    : "cannot access");
            if (!args->list) {
                quote_str_(params, str);
                return true;
            }
        }
    }

    if (is_dir && !args->directory) {
        return add_operand_(params, &params->dir_queue, str, dir_st);
    }

    return add_operand_(params, &params->operand_files, str, &st);
}

static bool add_operand_(t_params *params, t_array *dst, const t_str *str,
                         const struct stat *st) {
    t_entry *entry = entry_new_path(str, st, true);
    if (!entry) {
        return false;
    }

    params->quote_padding = params->quote_padding || entry->name_scan.quote;
    if (!array_append(dst, entry)) {
        entry_free(entry);
        return false;
    }

    return true;
}

static void quote_str_(t_params *params, const t_str *str) {
    t_shell_scan scan;

    shell_scan_str(str, &scan);
    params->quote_padding = params->quote_padding || scan.quote;
}

static bool load_directory_entries_(t_params *params, DIR *d,
                                    const t_str *dir_path) {
    clear_directory_entries_(params);

    const struct dirent *dp;
    while ((dp = readdir(d))) {
        const unsigned char dtype = dp->d_type;
        const mode_t mode = dtype_to_mode_(dtype);
        const bool need_lstat = params->args->list || params->args->sort_time ||
                                params->args->color || mode == 0;

        if (!params->args->all && dp->d_name[0] == '.') {
            continue;
        }

        t_entry *entry = entry_new_dirent(&params->temp_arena, dp);
        if (!entry) {
            return false;
        }

        params->dir_quoted = params->dir_quoted || entry->name_scan.quote;
        entry->st.st_mode = mode;
        if (need_lstat) {
            if (!entry_build_path(&params->temp_arena, entry, dir_path)) {
                return false;
            }

            if (lstat(entry->path->str, &entry->st) < 0) {
                print_error_(&params->out, entry->path, errno, "cannot access");
                entry->stat_unavailable = true;
                set_exit_(params, 1);
            }
        }

        if (!array_append(&params->current_entries, entry)) {
            return false;
        }
    }

    return true;
}

static void clear_directory_entries_(t_params *params) {
    array_clear(&params->current_entries);
    params->dir_quoted = false;
    arena_clear(&params->temp_arena);
}

static bool queue_recursive_dirs_(t_params *params, const t_str *parent_path) {
    if (params->args->recursive && params->current_entries.len) {
        size_t index = params->current_entries.len;
        while (index > 0) {
            --index;
            t_entry *entry = params->current_entries.data[index];
            if (entry->stat_unavailable || !S_ISDIR(entry->st.st_mode)) {
                continue;
            }

            if (!ft_strncmp(entry->name->str, ".", entry->name->len) ||
                !ft_strncmp(entry->name->str, "..", entry->name->len)) {
                continue;
            }

            if (!entry_build_path(&params->temp_arena, entry, parent_path)) {
                return false;
            }

            t_entry *dir_entry = entry_new_path(entry->path, &entry->st, false);
            if (!dir_entry) {
                return false;
            }

            if (!array_append(&params->dir_queue, dir_entry)) {
                entry_free(dir_entry);
                return false;
            }
        }
    }

    return true;
}

static void cleanup_process_(t_params *params) {
    clear_directory_entries_(params);
    array_clear_with(&params->operand_files, entry_del);
    array_clear_with(&params->dir_queue, entry_del);
    array_destroy(&params->current_entries);
    array_destroy(&params->operand_files);
    array_destroy(&params->dir_queue);
    arena_release(&params->temp_arena);
}

static void print_error_(t_out *out, const t_str *path, const int e,
                         const char *prefix) {
    char buf[1024];
    t_out err;
    t_shell_scan scan;

    out_flush(out);
    out_init(&err, buf, sizeof(buf), STDERR_FILENO);
    shell_scan_str(path, &scan);

    out_str(&err, "ft_ls: ");
    out_str(&err, prefix);
    out_put(&err, " ", 1);
    if (scan.quote) {
        shell_escape_put(&err, path, scan.quote, false);
    } else {
        out_put(&err, "'", 1);
        out_put(&err, path->str, path->len);
        out_put(&err, "'", 1);
    }
    out_str(&err, ": ");
    out_str(&err, strerror(e));
    out_put(&err, "\n", 1);
    out_flush(&err);
}

static mode_t dtype_to_mode_(const unsigned char dtype) {
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

static void set_exit_(t_params *params, int code) {
    if (code > params->exit_code) {
        params->exit_code = code;
    }
}
