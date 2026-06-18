#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include "../include/ft_array.h"
#include "../include/ft_entry.h"
#include "../include/ft_printer.h"

#include "../libft/include/libft.h"

#include "ft_arena.h"
#include "ft_path_scratch.h"
#include "ft_printer_helper.h"
#include "ft_printer_list.h"
#include "ft_printer_list_scratch.h"
#include "ft_shell_escape.h"

#ifndef CACHE_SIZE
#define CACHE_SIZE UINT64_C(8)
#endif // ifndef CACHE_SIZE //

#ifndef PERMISSION_SIZE
#define PERMISSION_SIZE UINT64_C(12)
#endif // ifndef PERMISSION_SIZE //

#ifndef DT_LEN
#define DT_LEN UINT64_C(13)
#endif // ifndef DT_LEN //

#ifndef LOGIN_NAME_MAX
#define LOGIN_NAME_MAX INT64_C(256)
#endif // ifndef LOGIN_NAME_MAX //

#ifndef PATH_MAX
#define PATH_MAX INT64_C(4096)
#endif // ifndef PATH_MAX //

#ifndef LS_RECENT_SECS
#define LS_RECENT_SECS ((time_t)(31556952 / 2))
#endif // ifndef LS_RECENT_SECS //

typedef struct {
    uint64_t total;
    uint64_t max_len_links;
    uint64_t max_len_sizes;
    uint64_t max_len_perm;
    bool have_quote;
} t_list_stats;

typedef struct s_file_info {
    t_str *perm;
    t_str *links;
    t_str *username;
    t_str *groupname;
    t_str *size;
    t_str *symlink;
    t_str *dt;
    uint64_t blocks;
} t_file_info;

typedef struct {
    char name[LOGIN_NAME_MAX];
    uint64_t id;
} t_id_cache_entry;

static uint64_t user_index = 0;
static t_id_cache_entry user_cache[CACHE_SIZE] = {0};

static uint64_t group_index = 0;
static t_id_cache_entry group_cache[CACHE_SIZE] = {0};

static bool prepare_list_infos_(Arena *arena, const t_array *entries,
                                t_file_info **infos);
static bool printer_list_with_scratch_(const t_print_request *req,
                                       Arena *scratch);
static void apply_list_width_context_(const t_array *context,
                                      const t_file_info *context_infos,
                                      t_list_stats *sizes);
static void update_list_stats_(t_list_stats *stats, const t_entry *entry,
                               const t_file_info *info);
static bool print_list_(const t_print_request *req, const t_file_info *infos,
                        const t_file_info *context_infos);
static bool left_pad_(t_str *out, uint64_t src_len, uint64_t max_size);
static bool put_uint_(t_str *out, uint64_t value);
static bool print_list_rows_(t_str *out, const t_array *array,
                             const t_file_info *infos,
                             const t_list_stats *sizes);
static bool fill_file_info_(Arena *arena, t_file_info *info,
                            const t_entry *entry);
static t_str *get_perm_(Arena *arena, const t_entry *entry);
static t_str *get_user_(Arena *arena, uid_t user_id);
static t_str *get_group_(Arena *arena, gid_t group_id);
static t_str *get_dt_(Arena *arena, const struct timespec *ctim);
static t_str *get_symlink_(Arena *arena, const t_entry *entry);
static char *cache_lookup_(t_id_cache_entry *cache, uint64_t id);
static void cache_store_(t_id_cache_entry *cache, uint64_t *next, uint64_t id,
                         const char *name);
static char file_type_char_(mode_t mode);
static char exec_char_(mode_t mode, mode_t exec_bit, mode_t special_bit,
                       char lower, char upper);

bool printer_list(const t_print_request *req) {
    Arena *scratch = arena_alloc(ARENA_SIZE);
    if (!scratch) {
        return false;
    }

    const bool ok = printer_list_with_scratch_(req, scratch);
    arena_release(scratch);
    return ok;
}

static bool printer_list_with_scratch_(const t_print_request *req,
                                       Arena *scratch) {
    t_file_info *infos = NULL;
    t_file_info *context_infos = NULL;

    if (!prepare_list_infos_(scratch, req->entries, &infos) ||
        !prepare_list_infos_(scratch, req->list_width_context,
                             &context_infos)) {
        return false;
    }

    return print_list_(req, infos, context_infos);
}

static bool prepare_list_infos_(Arena *arena, const t_array *entries,
                                t_file_info **infos) {
    if (!entries) {
        *infos = NULL;
        return true;
    }

    *infos = arena_push(arena, sizeof(**infos) * entries->len);
    if (!*infos) {
        return false;
    }

    for (uint64_t index = 0; index < entries->len; ++index) {
        const t_entry *entry = entries->data[index];

        if (!fill_file_info_(arena, &(*infos)[index], entry)) {
            return false;
        }
    }

    return true;
}

static void apply_list_width_context_(const t_array *context,
                                      const t_file_info *context_infos,
                                      t_list_stats *sizes) {
    if (!context || !context_infos) {
        return;
    }

    for (uint64_t index = 0; index < context->len; ++index) {
        const t_entry *entry = context->data[index];
        const t_file_info *info = &context_infos[index];
        if (!entry) {
            continue;
        }

        if (info->links->len > sizes->max_len_links) {
            sizes->max_len_links = info->links->len;
        }

        if (info->size->len > sizes->max_len_sizes) {
            sizes->max_len_sizes = info->size->len;
        }
    }
}

static void update_list_stats_(t_list_stats *stats, const t_entry *entry,
                               const t_file_info *info) {
    if (info->links->len > stats->max_len_links) {
        stats->max_len_links = info->links->len;
    }

    if (info->size->len > stats->max_len_sizes) {
        stats->max_len_sizes = info->size->len;
    }

    if (info->perm->len > stats->max_len_perm) {
        stats->max_len_perm = info->perm->len;
    }

    if (!stats->have_quote) {
        t_shell_scan scan;
        shell_scan_str(entry->name, &scan);
        stats->have_quote = scan.quote != '\0';
    }

    stats->total += info->blocks;
}

static bool print_list_(const t_print_request *req, const t_file_info *infos,
                        const t_file_info *context_infos) {
    t_list_stats sizes = {0};
    for (uint64_t index = 0; index < req->entries->len; ++index) {
        const t_entry *entry = req->entries->data[index];
        update_list_stats_(&sizes, entry, &infos[index]);
    }
    apply_list_width_context_(req->list_width_context, context_infos, &sizes);

    sizes.have_quote =
        sizes.have_quote || context_needs_padding(req->quote_padding_context);

    if (!put_dir_header(req->buffer, req->dir_header)) {
        return false;
    }

    if (req->print_total) {
        if (!put_mem(req->buffer, "total ", 6) ||
            !put_uint_(req->buffer, (sizes.total + 1) / 2) ||
            !put_mem(req->buffer, "\n", 1)) {
            return false;
        }
    }

    return print_list_rows_(req->buffer, req->entries, infos, &sizes);
}

static bool left_pad_(t_str *out, const uint64_t src_len,
                      const uint64_t max_size) {
    uint64_t count = max_size - src_len;

    while (count) {
        if (out->len == out->cap - 1 && !flush_str(out)) {
            return false;
        }

        const uint64_t avail = (out->cap - 1) - out->len;
        const uint64_t to_fill = count < avail ? count : avail;
        ft_memset(out->str + out->len, ' ', (size_t)to_fill);
        out->len += to_fill;
        out->str[out->len] = '\0';
        count -= to_fill;
    }

    return true;
}

static bool put_uint_(t_str *out, uint64_t value) {
    char digits[32];
    size_t index = sizeof(digits);

    do {
        digits[--index] = (char)('0' + (value % 10));
        value /= 10;
    } while (value > 0);

    return put_mem(out, digits + index, (uint64_t)(sizeof(digits) - index));
}

static bool print_list_rows_(t_str *out, const t_array *array,
                             const t_file_info *infos,
                             const t_list_stats *sizes) {
    for (uint64_t index = 0; index < array->len; ++index) {
        const t_entry *entry = array->data[index];
        const t_file_info *info = &infos[index];
        t_shell_scan scan;

        shell_scan_str(entry->name, &scan);

        if (!put_mem(out, info->perm->str, info->perm->len) ||
            !left_pad_(out, info->perm->len, sizes->max_len_perm) ||
            !put_mem(out, " ", 1) ||
            !left_pad_(out, info->links->len, sizes->max_len_links) ||
            !put_mem(out, info->links->str, info->links->len) ||
            !put_mem(out, " ", 1) ||
            !put_mem(out, info->username->str, info->username->len) ||
            !put_mem(out, " ", 1) ||
            !put_mem(out, info->groupname->str, info->groupname->len) ||
            !put_mem(out, " ", 1) ||
            !left_pad_(out, info->size->len, sizes->max_len_sizes) ||
            !put_mem(out, info->size->str, info->size->len) ||
            !put_mem(out, " ", 1) ||
            !put_mem(out, info->dt->str, info->dt->len) ||
            !put_mem(out, " ", 1) ||
            !put_shell_escaped(out, entry->name, scan.quote,
                               sizes->have_quote)) {
            return false;
        }

        if (info->symlink && info->symlink->len > 0) {
            if (!put_mem(out, " -> ", 4) ||
                !put_mem(out, info->symlink->str, info->symlink->len)) {
                return false;
            }
        }

        if (!put_mem(out, "\n", 1)) {
            return false;
        }
    }

    return true;
}

static bool fill_file_info_(Arena *arena, t_file_info *info,
                            const t_entry *entry) {
    if (entry->stat_unavailable) {
        info->perm = get_perm_(arena, entry);
        if (!info->perm) {
            return false;
        }

        info->links = printer_scratch_str_from_cstr(arena, "?");
        if (!info->links) {
            return false;
        }

        info->username = printer_scratch_str_from_cstr(arena, "?");
        if (!info->username) {
            return false;
        }

        info->groupname = printer_scratch_str_from_cstr(arena, "?");
        if (!info->groupname) {
            return false;
        }

        info->size = printer_scratch_str_from_cstr(arena, "?");
        if (!info->size) {
            return false;
        }

        info->dt = printer_scratch_str_from_cstr(arena, "           ?");
        if (!info->dt) {
            return false;
        }

        info->symlink = printer_scratch_str_new(arena, 1);
        if (!info->symlink) {
            return false;
        }

        info->blocks = 0;
        return true;
    }

    info->perm = get_perm_(arena, entry);
    if (!info->perm) {
        return false;
    }

    info->links = printer_scratch_str_from_uint(arena, entry->st.st_nlink);
    if (!info->links) {
        return false;
    }

    info->username = get_user_(arena, entry->st.st_uid);
    if (!info->username) {
        return false;
    }

    info->groupname = get_group_(arena, entry->st.st_gid);
    if (!info->groupname) {
        return false;
    }

    info->size =
        printer_scratch_str_from_uint(arena, (uint64_t)entry->st.st_size);
    if (!info->size) {
        return false;
    }

    info->dt = get_dt_(arena, &entry->st.st_mtim);
    if (!info->dt) {
        return false;
    }

    info->symlink = get_symlink_(arena, entry);
    if (!info->symlink) {
        return false;
    }

    info->blocks = (uint64_t)entry->st.st_blocks;
    return true;
}

static t_str *get_perm_(Arena *arena, const t_entry *entry) {
    t_str *str = printer_scratch_str_new(arena, PERMISSION_SIZE);

    if (!str) {
        return NULL;
    }

    uint64_t index = 0;
    str->str[index++] = file_type_char_(entry->st.st_mode);
    if (entry->stat_unavailable) {
        while (index < 10) {
            str->str[index++] = '?';
        }

        str->len = index;
        str->str[index] = '\0';
        return str;
    }

    str->str[index++] = entry->st.st_mode & S_IRUSR ? 'r' : '-';
    str->str[index++] = entry->st.st_mode & S_IWUSR ? 'w' : '-';
    str->str[index++] =
        exec_char_(entry->st.st_mode, S_IXUSR, S_ISUID, 's', 'S');
    str->str[index++] = entry->st.st_mode & S_IRGRP ? 'r' : '-';
    str->str[index++] = entry->st.st_mode & S_IWGRP ? 'w' : '-';
    str->str[index++] =
        exec_char_(entry->st.st_mode, S_IXGRP, S_ISGID, 's', 'S');
    str->str[index++] = entry->st.st_mode & S_IROTH ? 'r' : '-';
    str->str[index++] = entry->st.st_mode & S_IWOTH ? 'w' : '-';
    str->str[index++] =
        exec_char_(entry->st.st_mode, S_IXOTH, S_ISVTX, 't', 'T');

    str->len = index;
    str->str[index] = '\0';
    return str;
}

static char file_type_char_(const mode_t mode) {
    if ((mode & S_IFMT) == S_IFREG) {
        return '-';
    }

    if ((mode & S_IFMT) == S_IFDIR) {
        return 'd';
    }

    if ((mode & S_IFMT) == S_IFLNK) {
        return 'l';
    }

    if ((mode & S_IFMT) == S_IFIFO) {
        return 'p';
    }

    if ((mode & S_IFMT) == S_IFSOCK) {
        return 's';
    }

    if ((mode & S_IFMT) == S_IFCHR) {
        return 'c';
    }

    if ((mode & S_IFMT) == S_IFBLK) {
        return 'b';
    }

    return '?';
}

static t_str *get_user_(Arena *arena, const uid_t user_id) {
    const char *name = cache_lookup_(user_cache, (uint64_t)user_id);
    if (name) {
        return printer_scratch_str_from_cstr(arena, name);
    }

    const struct passwd *pwd = getpwuid(user_id);
    t_str *new_str = NULL;
    if (pwd) {
        new_str = printer_scratch_str_from_cstr(arena, pwd->pw_name);
        if (!new_str) {
            return NULL;
        }
        cache_store_(user_cache, &user_index, (uint64_t)user_id, pwd->pw_name);
    } else {
        const int err = errno;

        new_str = printer_scratch_str_from_uint(arena, (uint64_t)user_id);
        if (!new_str) {
            return NULL;
        }

        if (!err) {
            cache_store_(user_cache, &user_index, (uint64_t)user_id,
                         new_str->str);
        }
    }

    return new_str;
}

static t_str *get_group_(Arena *arena, const gid_t group_id) {
    const char *name = cache_lookup_(group_cache, (uint64_t)group_id);
    if (name) {
        return printer_scratch_str_from_cstr(arena, name);
    }

    const struct group *grp = getgrgid(group_id);
    t_str *new_str = NULL;
    if (grp) {
        new_str = printer_scratch_str_from_cstr(arena, grp->gr_name);
        if (!new_str) {
            return NULL;
        }

        cache_store_(group_cache, &group_index, (uint64_t)group_id,
                     grp->gr_name);
    } else {
        const int err = errno;

        new_str = printer_scratch_str_from_uint(arena, (uint64_t)group_id);
        if (!new_str) {
            return NULL;
        }

        if (!err) {
            cache_store_(group_cache, &group_index, (uint64_t)group_id,
                         new_str->str);
        }
    }

    return new_str;
}

static t_str *get_dt_(Arena *arena, const struct timespec *ctim) {
    Arena_Mark mark = arena_get_mark(arena);
    t_str *new_str = printer_scratch_str_new(arena, DT_LEN);
    if (!new_str) {
        return NULL;
    }

    const time_t stamp = ctim->tv_sec;
    const char *dt = ctime(&stamp);
    if (!dt) {
        arena_pop_to_mark(arena, mark);
        return NULL;
    }

    const time_t now = time(NULL);
    if (now == (time_t)-1) {
        arena_pop_to_mark(arena, mark);
        return NULL;
    }

    bool recent = false;
    if (stamp <= now) {
        recent = (now - stamp) < LS_RECENT_SECS;
    }

    if (recent) {
        ft_memcpy(new_str->str, dt + 4, 12);
    } else {
        ft_memcpy(new_str->str, dt + 4, 7);
        new_str->str[7] = ' ';
        ft_memcpy(new_str->str + 8, dt + 20, 4);
    }

    new_str->len = 12;
    new_str->str[12] = '\0';
    return new_str;
}

static t_str *get_symlink_(Arena *arena, const t_entry *entry) {
    if (!S_ISLNK(entry->st.st_mode)) {
        return printer_scratch_str_new(arena, 1);
    }

    if (!entry->path) {
        return NULL;
    }

    return path_read_symlink_scratch(arena, entry->path,
                                     (uint64_t)entry->st.st_size, NULL);
}

static char *cache_lookup_(t_id_cache_entry *cache, const uint64_t id) {
    for (uint64_t index = 0; index < CACHE_SIZE; ++index) {
        if (!*cache[index].name) {
            continue;
        }

        if (cache[index].id == id) {
            return cache[index].name;
        }
    }

    return NULL;
}

static void cache_store_(t_id_cache_entry *cache, uint64_t *next,
                         const uint64_t id, const char *name) {
    const uint64_t index = *next % CACHE_SIZE;
    cache[index].id = id;
    ft_strlcpy(cache[index].name, name, LOGIN_NAME_MAX);
    ++(*next);
}

static char exec_char_(const mode_t mode, const mode_t exec_bit,
                       const mode_t special_bit, const char lower,
                       const char upper) {
    const bool has_exec = (mode & exec_bit) != 0;
    const bool has_special = (mode & special_bit) != 0;

    if (!has_special) {
        return has_exec ? 'x' : '-';
    }

    return has_exec ? lower : upper;
}
