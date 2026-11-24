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
    t_arena *temp_arena;
    t_array dir_queue;
    t_array operand_files;
    t_array current_entries;
    t_out out;
    bool fatal;
    bool quote_padding;
} t_params;

typedef enum {
    OPERAND_SKIP,
    OPERAND_FILE,
    OPERAND_FATAL,
} t_operand_state;

static bool run_listing_(t_params *params, const t_array *array,
                         int *exit_code);
static bool print_operand_files_(t_params *params, const t_print_request *req,
                                 bool *printed_files);
static bool process_queue_(t_params *params, const t_array *array,
                           t_print_request *req, int *exit_code,
                           bool printed_files);
static bool load_directory_entries_(t_params *params, const t_entry *path,
                                    int *exit_code);
static bool queue_recursive_dirs_(t_params *params, const t_str *parent_path);
static bool collect_operands_(t_params *params, const t_array *array,
                              int *exit_code);
static void clear_directory_entries_(t_params *params);
static t_operand_state classify_operand_(t_params *params, const t_str *str,
                                         struct stat *st, int *exit_code);
static void cleanup_process_(t_params *params);
static bool queue_operand_dir_(t_params *params, const t_str *str,
                               const struct stat *st);
static void print_error_(t_out *out, const t_str *path, int e,
                         const char *prefix);
static mode_t dtype_to_mode_(unsigned char dtype);

int process(const t_args *args, const t_array *array) {
    static char out_buf[OUTPUT_BUFFER_CAP];
    t_params params = {0};
    int exit_code = 0;

    params.args = args;
    out_init(&params.out, out_buf, sizeof(out_buf), STDOUT_FILENO);
    params.temp_arena = arena_alloc(ARENA_SIZE);
    if (!params.temp_arena) {
        exit_code = 2;
        goto cleanup;
    }

    if (!array_init(&params.dir_queue, ARRAY_SIZE) ||
        !array_init(&params.operand_files, ARRAY_SIZE) ||
        !array_init(&params.current_entries, ARRAY_SIZE)) {
        exit_code = 2;
        goto cleanup;
    }

    if (!collect_operands_(&params, array, &exit_code)) {
        exit_code = 2;
        goto cleanup;
    }

    if (!run_listing_(&params, array, &exit_code)) {
        exit_code = 2;
    }

cleanup:
    cleanup_process_(&params);
    return exit_code;
}

static bool run_listing_(t_params *params, const t_array *array,
                         int *exit_code) {
    bool printed_files = false;
    bool ok = false;

    t_print_request req = {.entries = &params->operand_files,
                           .list_width_context = &params->dir_queue,
                           .dir_header = NULL,
                           .buffer = &params->out,
                           .arena = arena_alloc(ARENA_SIZE),
                           .quote_padding = params->quote_padding,
                           .list_mode = params->args->list,
                           .print_total = false,
                           .no_owner = params->args->no_owner,
                           .access_time = params->args->access_time,
                           .no_group = params->args->no_group,
                           .color = params->args->color,
                           .is_stdout = true,
                           .term_size = get_terminal_width()};

    if (!req.arena) {
        goto cleanup;
    }

    if (!isatty(STDOUT_FILENO)) {
        req.is_stdout = false;
    }

    if (!print_operand_files_(params, &req, &printed_files)) {
        goto cleanup;
    }

    if (params->args->directory) {
        ok = true;
        goto cleanup;
    }

    req.print_total = true;
    req.quote_padding = false;
    req.list_width_context = NULL;
    if (!process_queue_(params, array, &req, exit_code, printed_files)) {
        goto cleanup;
    }

    ok = true;
cleanup:
    out_flush(&params->out);
    if (req.arena) {
        arena_release(req.arena);
    }

    return ok && !params->out.failed;
}

static bool print_operand_files_(t_params *params, const t_print_request *req,
                                 bool *printed_files) {
    if (!params->operand_files.len) {
        return true;
    }

    bool ok = sort(&params->operand_files, params->args, params->args->reverse);
    if (ok) {
        ok = printer(req);
        *printed_files = ok;
    }

    array_clear_with(&params->operand_files, entry_del);
    return ok;
}

static bool process_queue_(t_params *params, const t_array *array,
                           t_print_request *req, int *exit_code,
                           const bool printed_files) {
    bool printed_dir = false;
    bool inserted_files_dirs_gap = false;
    const bool print_dir_path = params->args->recursive || array->len > 1;
    t_entry *dir_path = NULL;

    if (params->args->unsort) {
        array_reverse(&params->dir_queue);
    }

    while (params->dir_queue.len && !params->out.failed) {
        dir_path = array_pop(&params->dir_queue);

        if (!inserted_files_dirs_gap && printed_files && dir_path->is_operand) {
            out_put(req->buffer, "\n", 1);
            inserted_files_dirs_gap = true;
        }

        if (!load_directory_entries_(params, dir_path, exit_code)) {
            clear_directory_entries_(params);
            if (params->fatal) {
                goto error;
            }
            entry_free(dir_path);
            dir_path = NULL;
            continue;
        }

        if (!sort(&params->current_entries, params->args,
                  params->args->reverse)) {
            goto error;
        }

        if (!queue_recursive_dirs_(params, dir_path->path)) {
            goto error;
        }

        if (printed_dir) {
            out_put(req->buffer, "\n", 1);
        }

        req->dir_header = print_dir_path ? dir_path->path : NULL;
        req->entries = &params->current_entries;
        if (!printer(req)) {
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

static bool collect_operands_(t_params *params, const t_array *array,
                              int *exit_code) {
    for (size_t index = 0; index < array->len; ++index) {
        struct stat st = {0};
        const t_str *str = array->data[index];

        const t_operand_state state =
            classify_operand_(params, str, &st, exit_code);

        switch (state) {
            case OPERAND_SKIP: {
                t_shell_scan scan;

                shell_scan_str(str, &scan);
                params->quote_padding = params->quote_padding || scan.quote;
                continue;
            }
            case OPERAND_FATAL: goto failed;
            case OPERAND_FILE: break;
        }

        if (S_ISDIR(st.st_mode) && !params->args->directory) {
            if (!queue_operand_dir_(params, str, &st)) {
                goto failed;
            }
            continue;
        }

        t_entry *entry = entry_new_file_operand(str, &st);
        if (!entry) {
            goto failed;
        }

        params->quote_padding = params->quote_padding || entry->name_scan.quote;
        if (!array_append(&params->operand_files, entry)) {
            entry_free(entry);
            goto failed;
        }
    }

    if (!sort(&params->dir_queue, params->args, !params->args->reverse)) {
        goto failed;
    }

    return true;
failed:
    *exit_code = 2;
    return false;
}

static t_operand_state classify_operand_(t_params *params, const t_str *str,
                                         struct stat *st, int *exit_code) {
    int e = 0;
    const t_arena_mark mark = arena_get_mark(params->temp_arena);
    t_operand_state state = OPERAND_FILE;

    if (lstat(str->str, st) < 0) {
        print_error_(&params->out, str, errno, "cannot access");
        *exit_code = 2;
        return OPERAND_SKIP;
    }

    bool is_dir_operand = S_ISDIR(st->st_mode);
    const struct stat *st_dir = st;
    struct stat st_target;
    if (params->args->directory && !params->args->list) {
        goto cleanup;
    }

    if (S_ISLNK(st->st_mode)) {
        if (!stat(str->str, &st_target) && S_ISDIR(st_target.st_mode)) {
            is_dir_operand = true;
            st_dir = &st_target;
        }

        if (!read_symlink(params->temp_arena, str, (size_t)st->st_size, &e)) {
            state = OPERAND_FATAL;
            goto cleanup;
        }

        if (e) {
            *exit_code = 2;
            if (params->args->list) {
                print_error_(&params->out, str, e, "cannot read symbolic link");
            } else {
                print_error_(&params->out, str, e, "cannot access");
                state = OPERAND_SKIP;
                goto cleanup;
            }
        }
    }

    if (is_dir_operand && !params->args->list && !params->args->directory) {
        if (!queue_operand_dir_(params, str, st_dir)) {
            state = OPERAND_FATAL;
            goto cleanup;
        }

        state = OPERAND_SKIP;
    }
cleanup:
    arena_pop_to_mark(params->temp_arena, mark);
    return state;
}

static bool load_directory_entries_(t_params *params, const t_entry *path,
                                    int *exit_code) {
    errno = 0;
    bool ok = false;
    DIR *d = opendir(path->path->str);
    if (!d) {
        print_error_(&params->out, path->path, errno, "cannot open directory");
        *exit_code = path->is_operand ? 2 : 1;
        return false;
    }

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

        t_entry *entry = entry_new_dirent(params->temp_arena, dp);
        if (!entry) {
            goto cleanup;
        }

        entry->st.st_mode = mode;
        if (need_lstat) {
            if (!entry_build_path(params->temp_arena, entry, path->path)) {
                goto cleanup;
            }

            if (lstat(entry->path->str, &entry->st) < 0) {
                print_error_(&params->out, entry->path, errno, "cannot access");
                entry->stat_unavailable = true;
                if (*exit_code != 2) {
                    *exit_code = 1;
                }
            }
        }

        if (!array_append(&params->current_entries, entry)) {
            goto cleanup;
        }
    }

    ok = true;
cleanup:
    closedir(d);

    if (!ok) {
        params->fatal = true;
        *exit_code = 2;
    }

    return ok;
}

static void clear_directory_entries_(t_params *params) {
    array_clear(&params->current_entries);
    if (params->temp_arena) {
        arena_clear(params->temp_arena);
    }
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

            if (!entry_build_path(params->temp_arena, entry, parent_path)) {
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

    if (params->temp_arena) {
        arena_release(params->temp_arena);
    }
}

static bool queue_operand_dir_(t_params *params, const t_str *str,
                               const struct stat *st) {
    t_entry *dir_entry = entry_new_path(str, st, true);
    if (!dir_entry) {
        return false;
    }

    params->quote_padding = params->quote_padding || dir_entry->name_scan.quote;
    if (!array_append(&params->dir_queue, dir_entry)) {
        entry_free(dir_entry);
        return false;
    }

    return true;
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
