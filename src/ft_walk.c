#include <dirent.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../include/ft_arena.h"
#include "../include/ft_array.h"
#include "../include/ft_entry.h"
#include "../include/ft_free_list.h"
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
    bool output_failed;
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
static bool queue_recursive_dirs_(t_params *params);
static bool collect_operands_(t_params *params, const t_array *array,
                              int *exit_code);
static void clear_directory_entries_(const t_params *params);
static void free_entry_array_(t_params *params, t_array *array);
static mode_t dtype_to_mode_(unsigned char dtype);
static bool needs_lstat_(const t_args *args, unsigned char dtype);
static t_operand_state classify_operand_(t_params *params, t_str *str,
                                         struct stat *st, int *exit_code);
static void cleanup_process_(t_params *params);
static bool print_path_error_(t_params *params, const t_str *str, int e,
                              const char *prefix);
static bool queue_operand_dir_(t_params *params, t_str *str,
                               const struct stat *st);
static t_entry *new_file_operand_(t_params *params, const t_str *str,
                                  const struct stat *st);
static bool sort_operand_dirs_(t_params *params);

void process(t_args *args, const t_array *array, int *exit_code) {
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

    params.dirs = init_array(&params.fl, ARRAY_SIZE);
    params.files = init_array(&params.fl, ARRAY_SIZE);

    if (!params.dirs || !params.files) {
        *exit_code = 2;
        goto cleanup;
    }

    if (!collect_operands_(&params, array, exit_code)) {
        *exit_code = 2;
        goto cleanup;
    }

    if (!run_listing_(&params, array, exit_code)) {
        *exit_code = 2;
    }

cleanup:
    cleanup_process_(&params);
}

static bool run_listing_(t_params *params, const t_array *array,
                         int *exit_code) {
    bool printed_files = false;
    params->out.len = 0;
    params->out.pos = 0;
    params->out.str[0] = '\0';
    params->output_failed = false;

    t_print_request req = {.entries = params->files,
                           .dir_header = NULL,
                           .buffer = &params->out,
                           .arena = params->temp_arena,
                           .quote_padding_context = array,
                           .list_width_context = params->dirs,
                           .list_mode = params->args->list,
                           .print_total = false};
    if (!print_operand_files_(params, &req, &printed_files)) {
        goto error;
    }

    req.print_total = true;
    req.quote_padding_context = NULL;
    req.list_width_context = NULL;
    if (!process_queue_(params, array, &req, exit_code, printed_files)) {
        goto error;
    }

    return flush_str(&params->out);
error:
    flush_str(&params->out);
    return false;
}

static bool print_operand_files_(t_params *params, const t_print_request *req,
                                 bool *printed_files) {
    bool state = false;
    if (!params->files->len) {
        return true;
    }

    if (!sort(&params->sort_scratch, params->files, params->args->reverse,
              params->args->time)) {
        goto cleanup;
    }

    if (!printer(req)) {
        goto cleanup;
    }

    *printed_files = true;
    state = true;
cleanup:
    free_entry_array_(params, params->files);
    return state;
}

static bool process_queue_(t_params *params, const t_array *array,
                           t_print_request *req, int *exit_code,
                           const bool printed_files) {
    bool printed_dir = false;
    bool inserted_files_dirs_gap = false;
    const bool print_dir_path = params->args->recursive || array->len > 1;
    t_entry *dir_path = NULL;

    while (params->dirs->len) {
        dir_path = pop_array(params->dirs);

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
            free_entry(&params->fl, dir_path);
            dir_path = NULL;
            continue;
        }

        if (!sort(&params->sort_scratch, params->files, params->args->reverse,
                  params->args->time)) {
            goto error;
        }

        if (!queue_recursive_dirs_(params)) {
            goto error;
        }

        if (printed_dir) {
            if (!put_mem(req->buffer, "\n", 1)) {
                goto error;
            }
        }

        req->dir_header = print_dir_path ? dir_path->path : NULL;
        req->entries = params->files;
        if (!printer(req)) {
            goto error;
        }

        printed_dir = true;
        clear_directory_entries_(params);
        free_entry(&params->fl, dir_path);
        dir_path = NULL;
    }

    return true;
error:
    free_entry(&params->fl, dir_path);
    return false;
}

static bool collect_operands_(t_params *params, const t_array *array,
                              int *exit_code) {
    struct stat st;
    for (uint64_t index = 0; index < array->len; ++index) {
        t_str *str = array->data[index];

        const t_operand_state state =
            classify_operand_(params, str, &st, exit_code);
        if (state == OPERAND_SKIP) {
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

        t_entry *entry = new_file_operand_(params, str, &st);
        if (!entry) {
            goto failed;
        }

        if (!append_array(params->files, entry)) {
            free_entry(&params->fl, entry);
            goto failed;
        }
    }

    if (!sort_operand_dirs_(params)) {
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
    const t_str *symlink = NULL;
    Arena_Mark mark = arena_get_mark(params->temp_arena);
    t_operand_state state = OPERAND_FILE;

    if (lstat(str->str, st) == -1) {
        e = errno;
        const char *prefix = "cannot access";
        if (!print_path_error_(params, str, e, prefix)) {
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

        symlink = arena_read_symlink(params->temp_arena, str,
                                     (uint64_t)st->st_size, &e);
        if (!symlink) {
            state = OPERAND_FATAL;
            goto cleanup;
        }

        if (e != 0) {
            *exit_code = 2;
            if (params->args->list) {
                if (!print_path_error_(params, str, e,
                                       "cannot read symbolic link")) {
                    state = OPERAND_FATAL;
                    goto cleanup;
                }
            } else {
                if (!print_path_error_(params, str, e, "cannot access")) {
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
    DIR *d = opendir(path->path->str);
    if (!d) {
        const int e = errno;
        if (!print_path_error_(params, path->path, e,
                               "cannot open directory")) {
            params->output_failed = true;
        }
        *exit_code = (path->is_operand ? 2 : 1);
        return false;
    }

    clear_directory_entries_(params);
    const struct dirent *dp;
    while ((dp = readdir(d)) != NULL) {
        const unsigned char dtype = dp->d_type;
        const bool need_lstat = needs_lstat_(params->args, dtype);
        if (!params->args->all && dp->d_name[0] == '.' &&
            dp->d_name[1] != '/') {
            continue;
        }

        t_entry *entry = arena_new_entry(params->temp_arena, path, dp);
        if (!entry) {
            goto cleanup;
        }

        entry->st.st_mode = dtype_to_mode_(dtype);
        if (need_lstat) {
            if (!arena_entry_path(params->temp_arena, entry)) {
                goto cleanup;
            }

            if (lstat(entry->path->str, &entry->st) == -1) {
                const int e = errno;
                if (!print_path_error_(params, entry->path, e,
                                       "cannot access")) {
                    params->output_failed = true;
                    goto cleanup;
                }

                entry->stat_unavailable = true;
                if (*exit_code != 2) {
                    *exit_code = 1;
                }
            }
        }

        if (!append_array(params->files, entry)) {
            goto cleanup;
        }
    }

    ok = true;
cleanup:
    closedir(d);

    if (!ok) {
        *exit_code = 2;
    }

    return ok;
}

static void clear_directory_entries_(const t_params *params) {
    clear_array(params->files);
    arena_clear(params->temp_arena);
}

static void free_entry_array_(t_params *params, t_array *array) {
    while (array->len) {
        const t_entry *entry = pop_array(array);
        free_entry(&params->fl, entry);
    }
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

static bool needs_lstat_(const t_args *args, const unsigned char dtype) {
    if (args->list || args->time) {
        return true;
    }

    return dtype_to_mode_(dtype) == 0;
}

static bool queue_recursive_dirs_(t_params *params) {
    if (params->args->recursive && params->files->len) {
        uint64_t index = params->files->len;
        while (index > 0) {
            --index;
            const t_entry *entry = params->files->data[index];
            if (!entry || !entry->name || entry->stat_unavailable ||
                !S_ISDIR(entry->st.st_mode)) {
                continue;
            }

            if (ft_strncmp(entry->name->str, ".", entry->name->len) == 0 ||
                ft_strncmp(entry->name->str, "..", entry->name->len) == 0) {
                continue;
            }

            t_entry *dir_entry = fl_dup_entry(&params->fl, entry, false);
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

static void cleanup_process_(t_params *params) {
    if (params->temp_arena) {
        arena_release(params->temp_arena);
    }

    free((void *)params->sort_scratch.data);
    fl_free_all(&params->fl);
}

static bool print_path_error_(t_params *params, const t_str *str, const int e,
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
            fl_free_str(fl, new_str);
            return true;
        }
    }

    ft_fprintf(STDERR_FILENO, "ft_ls: %s '%s': %s\n", prefix, str->str, msg);
    return true;
}

static bool queue_operand_dir_(t_params *params, t_str *str,
                               const struct stat *st) {
    const t_entry src = {
        .name = str,
        .path = str,
        .st = *st,
    };

    t_entry *dir_entry = fl_dup_entry(&params->fl, &src, true);
    if (!dir_entry) {
        return false;
    }

    if (!append_array(params->dirs, dir_entry)) {
        free_entry(&params->fl, dir_entry);
        return false;
    }

    return true;
}

static t_entry *new_file_operand_(t_params *params, const t_str *str,
                                  const struct stat *st) {
    t_entry *entry = fl_alloc(&params->fl, sizeof(*entry));
    if (!entry) {
        return NULL;
    }

    *entry = (t_entry){0};
    entry->name = fl_dup_str(&params->fl, str);
    if (!entry->name) {
        goto failed;
    }

    entry->path = entry->name;
    entry->st = *st;
    entry->is_operand = false;

    return entry;

failed:
    free_entry(&params->fl, entry);
    return NULL;
}

static bool sort_operand_dirs_(t_params *params) {
    if (!params->dirs->len) {
        return true;
    }

    if (!sort(&params->sort_scratch, params->dirs, !params->args->reverse,
              params->args->time)) {
        return false;
    }

    return true;
}
