#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

#include "./ft_arena.h"
#include "./ft_file_info.h"
#include "./ft_ls.h"
#include "./ft_out.h"
#include "./ft_printer.h"
#include "./ft_printer_helper.h"
#include "./ft_str.h"

typedef struct {
    size_t total;
    size_t max_len_links;
    size_t max_len_sizes;
    size_t max_len_user;
    size_t max_len_group;
    size_t max_len_major;
    size_t max_len_minor;
    size_t max_len_perm;
    bool have_quote;
} t_list_stats;

static void apply_width_(const t_array *context, t_list_stats *sizes);
static void update_stats_(t_list_stats *stats, const t_entry *entry,
                          const t_file_info *info);
static size_t dev_width_(const t_list_stats *sizes);
static size_t size_width_(const t_list_stats *sizes);
static void print_list_(const t_print_request *req, const t_file_info *infos);
static void put_uint_(t_out *out, size_t value);
static void put_padded_(t_out *out, const t_str *str, size_t width);
static void put_right_(t_out *out, const t_str *str, size_t width);
static void print_list_rows_(const t_print_request *req,
                             const t_file_info *infos,
                             const t_list_stats *sizes);

bool printer_list(const t_print_request *req) {
    t_file_info *infos = NULL;
    const bool ok =
        prepare_list_infos(req->arena, req->entries, &infos, req->access_time);

    if (ok) {
        print_list_(req, infos);
    }

    arena_clear(req->arena);
    return ok;
}

static void apply_width_(const t_array *context, t_list_stats *sizes) {
    if (!context) {
        return;
    }

    for (size_t index = 0; index < context->len; ++index) {
        const t_entry *entry = context->data[index];
        const size_t links_len =
            entry->stat_unavailable ? 1 : str_uint_len(entry->st.st_nlink);

        if (links_len > sizes->max_len_links) {
            sizes->max_len_links = links_len;
        }

        if (!entry->stat_unavailable &&
            (S_ISBLK(entry->st.st_mode) || S_ISCHR(entry->st.st_mode))) {
            const size_t major_len =
                str_uint_len((size_t)major(entry->st.st_rdev));
            const size_t minor_len =
                str_uint_len((size_t)minor(entry->st.st_rdev));

            if (major_len > sizes->max_len_major) {
                sizes->max_len_major = major_len;
            }

            if (minor_len > sizes->max_len_minor) {
                sizes->max_len_minor = minor_len;
            }
        } else {
            const size_t size_len =
                entry->stat_unavailable
                    ? 1
                    : str_uint_len((size_t)entry->st.st_size);

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

static size_t dev_width_(const t_list_stats *sizes) {
    if (!sizes->max_len_major && !sizes->max_len_minor) {
        return 0;
    }

    return sizes->max_len_major + 2 + sizes->max_len_minor;
}

static size_t size_width_(const t_list_stats *sizes) {
    const size_t dev_width = dev_width_(sizes);
    return sizes->max_len_sizes > dev_width ? sizes->max_len_sizes : dev_width;
}

static void print_list_(const t_print_request *req, const t_file_info *infos) {
    t_list_stats sizes = {0};
    for (size_t index = 0; index < req->entries->len; ++index) {
        const t_entry *entry = req->entries->data[index];
        update_stats_(&sizes, entry, &infos[index]);
    }

    apply_width_(req->list_width_context, &sizes);
    if (!sizes.have_quote) {
        sizes.have_quote = req->quote_padding;
    }

    put_dir_header(req->buffer, req->dir_header, req->is_stdout);

    if (req->print_total) {
        out_put(req->buffer, "total ", 6);
        put_uint_(req->buffer, (sizes.total + 1) / 2);
        out_put(req->buffer, "\n", 1);
    }

    print_list_rows_(req, infos, &sizes);
}

static void put_uint_(t_out *out, const size_t value) {
    char digits[32];
    t_str str;

    str_init(&str, digits, sizeof(digits) - 1);
    str_copy_uint(&str, value);
    out_put(out, str.str, str.len);
}

static void put_padded_(t_out *out, const t_str *str, const size_t width) {
    out_put(out, str->str, str->len);
    out_pad(out, ' ', width - str->len);
}

static void put_right_(t_out *out, const t_str *str, const size_t width) {
    out_pad(out, ' ', width - str->len);
    out_put(out, str->str, str->len);
}

static void print_list_rows_(const t_print_request *req,
                             const t_file_info *infos,
                             const t_list_stats *sizes) {
    t_out *out = req->buffer;

    for (size_t index = 0; index < req->entries->len; ++index) {
        const t_entry *entry = req->entries->data[index];
        const t_file_info *info = &infos[index];

        put_padded_(out, info->perm, sizes->max_len_perm);
        out_put(out, " ", 1);
        put_right_(out, info->links, sizes->max_len_links);
        out_put(out, " ", 1);

        if (!req->no_owner) {
            put_padded_(out, info->username, sizes->max_len_user);
            out_put(out, " ", 1);
        }

        if (!req->no_group) {
            put_padded_(out, info->groupname, sizes->max_len_group);
            out_put(out, " ", 1);
        }

        if (info->major) {
            out_pad(out, ' ', size_width_(sizes) - dev_width_(sizes));
            put_right_(out, info->major, sizes->max_len_major);
            out_put(out, ", ", 2);
            put_right_(out, info->minor, sizes->max_len_minor);
        } else {
            put_right_(out, info->size, size_width_(sizes));
        }

        out_put(out, " ", 1);
        out_put(out, info->dt->str, info->dt->len);
        out_put(out, " ", 1);
        put_entry_name(out, entry, sizes->have_quote, req->color,
                       req->is_stdout);

        if (info->symlink && info->symlink->len > 0) {
            out_put(out, " -> ", 4);
            out_put(out, info->symlink->str, info->symlink->len);
        }

        out_put(out, "\n", 1);
    }
}
