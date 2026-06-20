#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../include/ft_array.h"
#include "../include/ft_parse.h"
#include "../include/ft_str.h"
#include "../include/ft_walk.h"

#include "../libft/include/libft.h"

#include "./ft_arena.h"
#include "./ft_path_scratch.h"
#include "./ft_printer.h"
#include "./ft_printer_helper.h"
#include "./ft_shell_escape.h"
#include "./ft_shell_scan.h"
#include "./ft_sort.h"
#include "./ft_walk_entry.h"

typedef struct {
    const t_args *args;
    Arena *temp_arena;
    t_sort_scratch sort_scratch;
    t_array dir_queue;
    t_array operand_files;
    t_array current_entries;
    char out_buf[OUTPUT_BUFFER_CAP];
    t_str out;
    bool output_failed;
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
static t_operand_state classify_operand_(t_params *params, t_str *str,
                                         struct stat *st, int *exit_code);
static void cleanup_process_(t_params *params);
static bool queue_operand_dir_(t_params *params, const t_str *str,
                               const struct stat *st);
static bool print_error_(t_str *out, const t_str *path, const int e,
                         const char *prefix, bool *output_failed);
static mode_t dtype_to_mode_(unsigned char dtype);

int process(const t_args *args, const t_array *array) {
    t_params params = {0};
    int exit_code = 0;

    params.args = args;
    params.out =
        (t_str){.str = params.out_buf, .cap = sizeof(params.out_buf), .len = 0};
    params.out.str[0] = '\0';
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
    params->out.len = 0;
    params->out.str[0] = '\0';
    params->output_failed = false;
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
                           .term_size = get_terminal_width()};

    if (!req.arena) {
        goto cleanup;
    }

    if (!print_operand_files_(params, &req, &printed_files)) {
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
    ok = flush_fd(&params->out, STDOUT_FILENO) && ok;
    if (req.arena) {
        arena_release(req.arena);
    }

    return ok;
}

static bool print_operand_files_(t_params *params, const t_print_request *req,
                                 bool *printed_files) {
    if (!params->operand_files.len) {
        return true;
    }

    bool ok = true;
    const bool sort_time = params->args->time ||
                           (params->args->access_time && !params->args->list);
    if (!params->args->unsort) {
        ok =
            sort(&params->sort_scratch, &params->operand_files,
                 params->args->reverse, sort_time, params->args->access_time) &&
            printer(req);
        if (ok) {
            *printed_files = true;
        }
    }

    array_clear_with(&params->operand_files, walk_entry_del);
    return ok;
}

static bool process_queue_(t_params *params, const t_array *array,
                           t_print_request *req, int *exit_code,
                           const bool printed_files) {
    bool printed_dir = false;
    bool inserted_files_dirs_gap = false;
    const bool print_dir_path = params->args->recursive || array->len > 1;
    const bool sort_time = params->args->time ||
                           (params->args->access_time && !params->args->list);
    t_entry *dir_path = NULL;

    if (params->args->unsort) {
       array_reverse(&params->dir_queue);
    }

    while (params->dir_queue.len) {
        dir_path = array_pop(&params->dir_queue);

        if (!inserted_files_dirs_gap && printed_files && dir_path->is_operand) {
            if (!put_mem(req->buffer, "\n", 1)) {
                goto error;
            }
            inserted_files_dirs_gap = true;
        }

        if (!load_directory_entries_(params, dir_path, exit_code)) {
            clear_directory_entries_(params);
            if (params->output_failed) {
                goto error;
            }
            walk_entry_free(dir_path);
            dir_path = NULL;
            continue;
        }

        if (!params->args->unsort &&
            !sort(&params->sort_scratch, &params->current_entries,
                  params->args->reverse, sort_time,
                  params->args->access_time)) {
            goto error;
        }

        if (!queue_recursive_dirs_(params, dir_path->path)) {
            goto error;
        }

        if (printed_dir) {
            if (!put_mem(req->buffer, "\n", 1)) {
                goto error;
            }
        }

        req->dir_header = print_dir_path ? dir_path->path : NULL;
        req->entries = &params->current_entries;
        if (!printer(req)) {
            goto error;
        }

        printed_dir = true;
        clear_directory_entries_(params);
        walk_entry_free(dir_path);
        dir_path = NULL;
    }

    return true;
error:
    walk_entry_free(dir_path);
    return false;
}

static bool collect_operands_(t_params *params, const t_array *array,
                              int *exit_code) {
    for (uint64_t index = 0; index < array->len; ++index) {
        struct stat st = {0};
        t_str *str = array->data[index];

        const t_operand_state state =
            classify_operand_(params, str, &st, exit_code);
        if (state == OPERAND_SKIP) {
            t_shell_scan scan;

            shell_scan_str(str, &scan);
            params->quote_padding = params->quote_padding || scan.quote != '\0';
            continue;
        }

        if (state == OPERAND_FATAL) {
            goto failed;
        }

        if (S_ISDIR(st.st_mode)) {
            if (!queue_operand_dir_(params, str, &st)) {
                goto failed;
            }
            continue;
        }

        t_entry *entry = walk_entry_new_file_operand(str, &st);
        if (!entry) {
            goto failed;
        }

        params->quote_padding =
            params->quote_padding || entry->name_scan.quote != '\0';

        if (!array_append(&params->operand_files, entry)) {
            walk_entry_free(entry);
            goto failed;
        }
    }

    const bool sort_time = params->args->time ||
                           (params->args->access_time && !params->args->list);
    if (params->dir_queue.len && !params->args->unsort &&
        !sort(&params->sort_scratch, &params->dir_queue, !params->args->reverse,
              sort_time, params->args->access_time)) {
        goto failed;
    }

    return true;
failed:
    *exit_code = 2;
    return false;
}

static t_operand_state classify_operand_(t_params *params, t_str *str,
                                         struct stat *st, int *exit_code) {
    int e = 0;
    Arena_Mark mark = arena_get_mark(params->temp_arena);
    t_operand_state state = OPERAND_FILE;

    if (lstat(str->str, st) == -1) {
        e = errno;
        const char *prefix = "cannot access";
        if (!print_error_(&params->out, str, e, prefix,
                          &params->output_failed)) {
            return OPERAND_FATAL;
        }

        *exit_code = 2;
        return OPERAND_SKIP;
    }

    bool is_dir_operand = S_ISDIR(st->st_mode);
    const struct stat *st_dir = st;
    struct stat st_target;
    if (S_ISLNK(st->st_mode)) {
        if (stat(str->str, &st_target) == 0 && S_ISDIR(st_target.st_mode)) {
            is_dir_operand = true;
            st_dir = &st_target;
        }

        if (!path_read_symlink(params->temp_arena, str, (uint64_t)st->st_size,
                               &e)) {
            state = OPERAND_FATAL;
            goto cleanup;
        }

        if (e != 0) {
            *exit_code = 2;
            if (params->args->list) {
                if (!print_error_(&params->out, str, e,
                                  "cannot read symbolic link",
                                  &params->output_failed)) {
                    state = OPERAND_FATAL;
                    goto cleanup;
                }
            } else {
                if (!print_error_(&params->out, str, e, "cannot access",
                                  &params->output_failed)) {
                    state = OPERAND_FATAL;
                    goto cleanup;
                }
                state = OPERAND_SKIP;
                goto cleanup;
            }
        }
    }

    if (is_dir_operand && !params->args->list) {
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
    bool hard_failure = false;
    DIR *d = opendir(path->path->str);
    if (!d) {
        const int e = errno;
        (void)print_error_(&params->out, path->path, e, "cannot open directory",
                           &params->output_failed);
        *exit_code = (path->is_operand ? 2 : 1);
        return false;
    }

    clear_directory_entries_(params);
    const struct dirent *dp;
    const bool sort_time = params->args->time ||
                           (params->args->access_time && !params->args->list);
    while ((dp = readdir(d)) != NULL) {
        const unsigned char dtype = dp->d_type;
        const mode_t mode = dtype_to_mode_(dtype);
        const bool need_lstat = params->args->list || sort_time || mode == 0;

        if (!params->args->all && dp->d_name[0] == '.' &&
            dp->d_name[1] != '/') {
            continue;
        }

        t_entry *entry = walk_entry_new_scratch_dirent(params->temp_arena, dp);
        if (!entry) {
            hard_failure = true;
            goto cleanup;
        }

        entry->st.st_mode = mode;
        if (need_lstat) {
            if (!walk_entry_build_path(params->temp_arena, entry, path->path)) {
                hard_failure = true;
                goto cleanup;
            }

            if (lstat(entry->path->str, &entry->st) == -1) {
                const int e = errno;
                if (!print_error_(&params->out, entry->path, e, "cannot access",
                                  &params->output_failed)) {
                    goto cleanup;
                }

                entry->stat_unavailable = true;
                if (*exit_code != 2) {
                    *exit_code = 1;
                }
            }
        }

        if (!array_append(&params->current_entries, entry)) {
            hard_failure = true;
            goto cleanup;
        }
    }

    ok = true;
cleanup:
    closedir(d);

    if (!ok) {
        if (hard_failure) {
            params->output_failed = true;
        }
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
        uint64_t index = params->current_entries.len;
        while (index > 0) {
            --index;
            t_entry *entry = params->current_entries.data[index];
            if (!entry || !entry->name || entry->stat_unavailable ||
                !S_ISDIR(entry->st.st_mode)) {
                continue;
            }

            if (ft_strncmp(entry->name->str, ".", entry->name->len) == 0 ||
                ft_strncmp(entry->name->str, "..", entry->name->len) == 0) {
                continue;
            }

            if (!entry->path && !walk_entry_build_path(params->temp_arena,
                                                       entry, parent_path)) {
                return false;
            }

            t_entry *dir_entry =
                walk_entry_new_owned_path(entry->path, &entry->st, false);
            if (!dir_entry) {
                return false;
            }

            if (!array_append(&params->dir_queue, dir_entry)) {
                walk_entry_free(dir_entry);
                return false;
            }
        }
    }

    return true;
}

static void cleanup_process_(t_params *params) {
    clear_directory_entries_(params);
    array_clear_with(&params->operand_files, walk_entry_del);
    array_clear_with(&params->dir_queue, walk_entry_del);
    array_destroy(&params->current_entries);
    array_destroy(&params->operand_files);
    array_destroy(&params->dir_queue);

    if (params->temp_arena) {
        arena_release(params->temp_arena);
    }

    free((void *)params->sort_scratch.data);
}

static bool queue_operand_dir_(t_params *params, const t_str *str,
                               const struct stat *st) {
    t_entry *dir_entry = walk_entry_new_owned_path(str, st, true);
    if (!dir_entry) {
        return false;
    }

    params->quote_padding =
        params->quote_padding || dir_entry->name_scan.quote != '\0';

    if (!array_append(&params->dir_queue, dir_entry)) {
        walk_entry_free(dir_entry);
        return false;
    }

    return true;
}

static bool print_error_(t_str *out, const t_str *path, const int e,
                         const char *prefix, bool *output_failed) {
    if (!flush_fd(out, STDOUT_FILENO)) {
        if (output_failed) {
            *output_failed = true;
        }
        return false;
    }

    char err_buf[1024];
    t_str err_out;
    str_init(&err_out, err_buf, sizeof(err_buf) - 1);

    t_shell_scan scan;
    shell_scan_str(path, &scan);
    const char *msg = strerror(e);
    if (scan.quote != '\0') {
        t_str *escaped = shell_escape_str(path, scan.quote);
        if (escaped) {
            const bool ok =
                put_mem_fd(&err_out, "ft_ls: ", sizeof("ft_ls: ") - 1,
                           STDERR_FILENO) &&
                put_mem_fd(&err_out, prefix, (uint64_t)ft_strlen(prefix),
                           STDERR_FILENO) &&
                put_mem_fd(&err_out, " ", 1, STDERR_FILENO) &&
                put_mem_fd(&err_out, escaped->str, escaped->len,
                           STDERR_FILENO) &&
                put_mem_fd(&err_out, ": ", 2, STDERR_FILENO) &&
                put_mem_fd(&err_out, msg, (uint64_t)ft_strlen(msg),
                           STDERR_FILENO) &&
                put_mem_fd(&err_out, "\n", 1, STDERR_FILENO) &&
                flush_fd(&err_out, STDERR_FILENO);
            str_free(escaped);
            return ok;
        }
    }

    return put_mem_fd(&err_out, "ft_ls: ", sizeof("ft_ls: ") - 1,
                      STDERR_FILENO) &&
           put_mem_fd(&err_out, prefix, (uint64_t)ft_strlen(prefix),
                      STDERR_FILENO) &&
           put_mem_fd(&err_out, " '", 2, STDERR_FILENO) &&
           put_mem_fd(&err_out, path->str, path->len, STDERR_FILENO) &&
           put_mem_fd(&err_out, "': ", 3, STDERR_FILENO) &&
           put_mem_fd(&err_out, msg, (uint64_t)ft_strlen(msg), STDERR_FILENO) &&
           put_mem_fd(&err_out, "\n", 1, STDERR_FILENO) &&
           flush_fd(&err_out, STDERR_FILENO);
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
