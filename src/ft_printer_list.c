#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

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
} t_list_stats;

static void print_list_(const t_print_request *req, const t_file_info *infos);
static void update_stats_(t_list_stats *stats, const t_file_info *info);
static size_t dev_width_(const t_list_stats *sizes);
static size_t size_width_(const t_list_stats *sizes);
static void put_uint_(t_out *out, size_t value);
static void put_padded_(t_out *out, const t_str *str, size_t width);
static void put_num_(t_out *out, const t_entry *entry, size_t value,
                     size_t width);
static void put_uint_right_(t_out *out, size_t value, size_t width);
static void print_list_rows_(const t_print_request *req,
                             const t_file_info *infos,
                             const t_list_stats *sizes);
static void update_num_widths_(t_list_stats *s, const t_entry *e);
static void max_(size_t *dst, size_t value);

bool printer_list(const t_print_request *req) {
    t_file_info *infos = NULL;
    const bool ok = prepare_list_infos(req, &infos);

    if (ok) {
        print_list_(req, infos);
    }

    return ok;
}

static void print_list_(const t_print_request *req, const t_file_info *infos) {
    t_list_stats sizes = {0};
    for (size_t index = 0; index < req->entries->len; ++index) {
        const t_entry *entry = req->entries->data[index];
        update_num_widths_(&sizes, entry);
        update_stats_(&sizes, &infos[index]);
        sizes.total += (size_t)entry->st.st_blocks;
    }

    const t_array *context = req->list_width_context;
    for (size_t index = 0; context && index < context->len; ++index) {
        update_num_widths_(&sizes, context->data[index]);
    }

    put_dir_header(req->buffer, req->dir_header, req->is_stdout);

    if (req->print_total) {
        out_put(req->buffer, "total ", 6);
        put_uint_(req->buffer, (sizes.total + 1) / 2);
        out_put(req->buffer, "\n", 1);
    }

    print_list_rows_(req, infos, &sizes);
}

static void update_stats_(t_list_stats *stats, const t_file_info *info) {
    if (info->username && info->username->len > stats->max_len_user) {
        stats->max_len_user = info->username->len;
    }

    if (info->groupname && info->groupname->len > stats->max_len_group) {
        stats->max_len_group = info->groupname->len;
    }

    if (info->perm_len > stats->max_len_perm) {
        stats->max_len_perm = info->perm_len;
    }
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

static void put_uint_(t_out *out, const size_t value) {
    char digits[32];
    t_str str;

    str_init(&str, digits);
    str_copy_uint(&str, value);
    out_put(out, str.str, str.len);
}

static void put_padded_(t_out *out, const t_str *str, const size_t width) {
    out_put(out, str->str, str->len);
    out_pad(out, ' ', width - str->len);
}

static void put_num_(t_out *out, const t_entry *entry, size_t value,
                     size_t width) {
    if (entry->stat_unavailable) {
        out_pad(out, ' ', width - 1);
        out_put(out, "?", 1);
    } else {
        put_uint_right_(out, value, width);
    }
}

static void put_uint_right_(t_out *out, const size_t value,
                            const size_t width) {
    out_pad(out, ' ', width - str_uint_len(value));
    put_uint_(out, value);
}

static void print_list_rows_(const t_print_request *req,
                             const t_file_info *infos,
                             const t_list_stats *sizes) {
    t_out *out = req->buffer;

    for (size_t index = 0; index < req->entries->len; ++index) {
        const t_entry *entry = req->entries->data[index];
        const t_file_info *info = &infos[index];
        const struct stat *st = &entry->st;

        out_put(out, info->perm, info->perm_len);
        out_pad(out, ' ', sizes->max_len_perm - info->perm_len);
        out_put(out, " ", 1);
        put_num_(out, entry, st->st_nlink, sizes->max_len_links);
        out_put(out, " ", 1);

        if (!req->no_owner) {
            put_padded_(out, info->username, sizes->max_len_user);
            out_put(out, " ", 1);
        }

        if (!req->no_group) {
            put_padded_(out, info->groupname, sizes->max_len_group);
            out_put(out, " ", 1);
        }

        if (S_ISBLK(st->st_mode) || S_ISCHR(st->st_mode)) {
            out_pad(out, ' ', size_width_(sizes) - dev_width_(sizes));
            put_uint_right_(out, major(st->st_rdev), sizes->max_len_major);
            out_put(out, ", ", 2);
            put_uint_right_(out, minor(st->st_rdev), sizes->max_len_minor);
        } else {
            put_num_(out, entry, (size_t)st->st_size, size_width_(sizes));
        }

        out_put(out, " ", 1);
        out_put(out, info->dt, DT_LEN - 1);
        out_put(out, " ", 1);
        put_entry_name(out, entry, req->quoted, req->color, req->is_stdout);

        if (info->symlink && info->symlink->len > 0) {
            out_put(out, " -> ", 4);
            put_link_target(out, info->symlink, req->is_stdout);
        }

        out_put(out, "\n", 1);
    }
}

static void update_num_widths_(t_list_stats *s, const t_entry *e) {
    max_(&s->max_len_links,
         e->stat_unavailable ? 1 : str_uint_len(e->st.st_nlink));

    if (S_ISBLK(e->st.st_mode) || S_ISCHR(e->st.st_mode)) {
        max_(&s->max_len_major, str_uint_len(major(e->st.st_rdev)));
        max_(&s->max_len_minor, str_uint_len(minor(e->st.st_rdev)));
    } else {
        max_(&s->max_len_sizes,
             e->stat_unavailable ? 1 : str_uint_len((size_t)e->st.st_size));
    }
}

static void max_(size_t *dst, const size_t value) {
    if (value > *dst) {
        *dst = value;
    }
}
