#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>

#include "../include/ft_str.h"

#include "./ft_arena.h"
#include "./ft_file_info.h"
#include "./ft_ls.h"
#include "./ft_printer.h"
#include "./ft_printer_helper.h"
#include "./ft_utils.h"

typedef struct {
    uint64_t total;
    uint64_t max_len_links;
    uint64_t max_len_sizes;
    uint64_t max_len_user;
    uint64_t max_len_group;
    uint64_t max_len_major;
    uint64_t max_len_minor;
    uint64_t max_len_perm;
    bool have_quote;
} t_list_stats;

static void apply_width_(const t_array *context, t_list_stats *sizes);
static void update_stats_(t_list_stats *stats, const t_entry *entry,
                          const t_file_info *info);
static uint64_t dev_width_(const t_list_stats *sizes);
static uint64_t size_width_(const t_list_stats *sizes);
static bool print_list_(const t_print_request *req, const t_file_info *infos);
static bool left_pad_(t_str *out, uint64_t src_len, uint64_t max_size);
static bool put_uint_(t_str *out, uint64_t value);
static bool print_list_rows_(const t_print_request *req,
                             const t_file_info *infos,
                             const t_list_stats *sizes);

bool printer_list(const t_print_request *req) {
    t_file_info *infos = NULL;
    bool ok = false;

    if (!prepare_list_infos(req->arena, req->entries, &infos,
                            req->access_time)) {
        goto cleanup;
    }

    ok = print_list_(req, infos);
cleanup:
    arena_clear(req->arena);
    return ok;
}

static void apply_width_(const t_array *context, t_list_stats *sizes) {
    if (!context) {
        return;
    }

    for (uint64_t index = 0; index < context->len; ++index) {
        const t_entry *entry = context->data[index];
        const uint64_t links_len =
            entry->stat_unavailable ? 1 : str_uint_len(entry->st.st_nlink);

        if (links_len > sizes->max_len_links) {
            sizes->max_len_links = links_len;
        }

        if (!entry->stat_unavailable &&
            (S_ISBLK(entry->st.st_mode) || S_ISCHR(entry->st.st_mode))) {
            const uint64_t major_len =
                str_uint_len((uint64_t)major(entry->st.st_rdev));
            const uint64_t minor_len =
                str_uint_len((uint64_t)minor(entry->st.st_rdev));

            if (major_len > sizes->max_len_major) {
                sizes->max_len_major = major_len;
            }

            if (minor_len > sizes->max_len_minor) {
                sizes->max_len_minor = minor_len;
            }
        } else {
            const uint64_t size_len =
                entry->stat_unavailable
                    ? 1
                    : str_uint_len((uint64_t)entry->st.st_size);

            if (size_len > sizes->max_len_sizes) {
                sizes->max_len_sizes = size_len;
            }
        }
    }
}

static void update_stats_(t_list_stats *stats, const t_entry *entry,
                          const t_file_info *info) {
    if (info->links->len > stats->max_len_links) {
        stats->max_len_links = info->links->len;
    }

    if (info->username->len > stats->max_len_user) {
        stats->max_len_user = info->username->len;
    }

    if (info->groupname->len > stats->max_len_group) {
        stats->max_len_group = info->groupname->len;
    }

    if (info->size) {
        if (info->size->len > stats->max_len_sizes) {
            stats->max_len_sizes = info->size->len;
        }
    } else {
        if (info->major && info->major->len > stats->max_len_major) {
            stats->max_len_major = info->major->len;
        }

        if (info->minor && info->minor->len > stats->max_len_minor) {
            stats->max_len_minor = info->minor->len;
        }
    }

    if (info->perm->len > stats->max_len_perm) {
        stats->max_len_perm = info->perm->len;
    }

    if (!stats->have_quote) {
        stats->have_quote = entry->name_scan.quote != '\0';
    }

    stats->total += info->blocks;
}

static uint64_t dev_width_(const t_list_stats *sizes) {
    if (!sizes->max_len_major && !sizes->max_len_minor) {
        return 0;
    }

    return sizes->max_len_major + 2 + sizes->max_len_minor;
}

static uint64_t size_width_(const t_list_stats *sizes) {
    const uint64_t dev_width = dev_width_(sizes);
    return sizes->max_len_sizes > dev_width ? sizes->max_len_sizes : dev_width;
}

static bool print_list_(const t_print_request *req, const t_file_info *infos) {
    t_list_stats sizes = {0};
    for (uint64_t index = 0; index < req->entries->len; ++index) {
        const t_entry *entry = req->entries->data[index];
        update_stats_(&sizes, entry, &infos[index]);
    }

    apply_width_(req->list_width_context, &sizes);
    if (!sizes.have_quote) {
        sizes.have_quote = req->quote_padding;
    }

    if (!put_dir_header(req->buffer, req->dir_header, req->is_stdout)) {
        return false;
    }

    if (req->print_total) {
        if (!put_mem(req->buffer, "total ", 6) ||
            !put_uint_(req->buffer, (sizes.total + 1) / 2) ||
            !put_mem(req->buffer, "\n", 1)) {
            return false;
        }
    }

    return print_list_rows_(req, infos, &sizes);
}

static bool left_pad_(t_str *out, const uint64_t src_len,
                      const uint64_t max_size) {
    uint64_t count = max_size - src_len;

    while (count) {
        if (out->len == out->cap - 1 && !flush_fd(out, STDOUT_FILENO)) {
            return false;
        }

        const uint64_t avail = out->cap - 1 - out->len;
        const uint64_t to_fill = count < avail ? count : avail;
        ft_memset(out->str + out->len, ' ', (size_t)to_fill);
        out->len += to_fill;
        out->str[out->len] = '\0';
        count -= to_fill;
    }

    return true;
}

static bool put_uint_(t_str *out, const uint64_t value) {
    char digits[32];
    t_str str;

    str_init(&str, digits, sizeof(digits) - 1);
    str_copy_uint(&str, value);

    return put_mem(out, str.str, str.len);
}

static bool print_list_rows_(const t_print_request *req,
                             const t_file_info *infos,
                             const t_list_stats *sizes) {
    for (uint64_t index = 0; index < req->entries->len; ++index) {
        const t_entry *entry = req->entries->data[index];
        const t_file_info *info = &infos[index];

        if (!put_mem(req->buffer, info->perm->str, info->perm->len) ||
            !left_pad_(req->buffer, info->perm->len, sizes->max_len_perm) ||
            !put_mem(req->buffer, " ", 1) ||
            !left_pad_(req->buffer, info->links->len, sizes->max_len_links) ||
            !put_mem(req->buffer, info->links->str, info->links->len) ||
            !put_mem(req->buffer, " ", 1)) {
            return false;
        }

        if (!req->no_owner &&
            (!put_mem(req->buffer, info->username->str, info->username->len) ||
             !left_pad_(req->buffer, info->username->len,
                        sizes->max_len_user) ||
             !put_mem(req->buffer, " ", 1))) {
            return false;
        }

        if (!req->no_group && (!put_mem(req->buffer, info->groupname->str,
                                        info->groupname->len) ||
                               !left_pad_(req->buffer, info->groupname->len,
                                          sizes->max_len_group) ||
                               !put_mem(req->buffer, " ", 1))) {
            return false;
        }

        if (info->major) {
            const uint64_t device_width = dev_width_(sizes);
            if (!left_pad_(req->buffer, device_width, size_width_(sizes)) ||
                !left_pad_(req->buffer, info->major->len,
                           sizes->max_len_major)) {
                return false;
            }

            if (!put_mem(req->buffer, info->major->str, info->major->len) ||
                !put_mem(req->buffer, ", ", 2) ||
                !left_pad_(req->buffer, info->minor->len, sizes->max_len_minor) ||
                !put_mem(req->buffer, info->minor->str, info->minor->len)) {
                return false;
            }
        } else {
            if (!left_pad_(req->buffer, info->size->len, size_width_(sizes)) ||
                !put_mem(req->buffer, info->size->str, info->size->len)) {
                return false;
            }
        }

        if (!put_mem(req->buffer, " ", 1) ||
            !put_mem(req->buffer, info->dt->str, info->dt->len) ||
            !put_mem(req->buffer, " ", 1) ||
            !put_entry_name(req->buffer, entry, sizes->have_quote, req->color,
                            req->is_stdout)) {
            return false;
        }

        if (info->symlink && info->symlink->len > 0) {
            if (!put_mem(req->buffer, " -> ", 4) ||
                !put_mem(req->buffer, info->symlink->str, info->symlink->len)) {
                return false;
            }
        }

        if (!put_mem(req->buffer, "\n", 1)) {
            return false;
        }
    }

    return true;
}
